/*
 *      Copyright (C) 2012-2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameClient.h"
#include "GameClientInput.h"
#include "addons/AddonManager.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "dialogs/GUIDialogOK.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "games/controllers/Controller.h"
#include "games/ports/PortManager.h"
#include "games/GameManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/InputManager.h"
#include "input/joysticks/DefaultJoystick.h" // for DEFAULT_CONTROLLER_ID
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/Peripherals.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <cstring>

using namespace ADDON;
using namespace GAME;
using namespace JOYSTICK;
using namespace XFILE;

#define EXTENSION_SEPARATOR          "|"
#define EXTENSION_WILDCARD           "*"
#define GAME_REGION_NTSC_STRING      "NTSC"
#define GAME_REGION_PAL_STRING       "PAL"
#define MAX_LAUNCH_FILE_CHOICES      20 // Show up to this many games when a directory is played
#define BUTTON_INDEX_MASK            0x01ff
#define DEMUX_READ_TIMEOUT_MS        1000

// --- NormalizeExtension ------------------------------------------------------

namespace
{
  /*
   * \brief Convert to lower case and canonicalize with a leading "."
   */
  std::string NormalizeExtension(const std::string& strExtension)
  {
    std::string ext = strExtension;

    if (!ext.empty() && ext != EXTENSION_WILDCARD)
    {
      StringUtils::ToLower(ext);

      if (ext[0] != '.')
        ext.insert(0, ".");
    }

    return ext;
  }
}

// --- CGameClient -------------------------------------------------------------

std::unique_ptr<CGameClient> CGameClient::FromExtension(ADDON::AddonProps props, const cp_extension_t* ext)
{
  /*
  string strPlatforms = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "platforms");
  if (!strPlatforms.empty())
  {
    Props().extrainfo.insert(make_pair("platforms", strPlatforms));
    SetPlatforms(strPlatforms);
  }
  */
  std::vector<std::string> extensions;
  std::string strExtensions = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "extensions");
  if (!strExtensions.empty())
    extensions = ParseExtensions(strExtensions);

  std::string strSupportsVFS = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "supports_vfs");
  bool bSupportsVFS = (strSupportsVFS == "true" || strSupportsVFS == "yes");

  std::string strSupportsStandalone = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "supports_no_game");
  bool bSupportsStandalone = (strSupportsStandalone == "true" || strSupportsStandalone == "yes");

  std::string strSupportsKeyboard = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "supports_keyboard");
  bool bSupportsKeyboard = (strSupportsKeyboard == "true" || strSupportsKeyboard == "yes");

  return std::unique_ptr<CGameClient>(new CGameClient(std::move(props), extensions, bSupportsVFS, bSupportsStandalone, bSupportsKeyboard));
}

CGameClient::CGameClient(ADDON::AddonProps props, const std::vector<std::string>& extensions, bool bSupportsVFS, bool bSupportsStandalone, bool bSupportsKeyboard)
  : CAddonDll<DllGameClient, GameClient, game_client_properties>(std::move(props)),
    m_apiVersion("0.0.0"),
    m_libraryProps(this, m_pInfo),
    m_bSupportsVFS(bSupportsVFS),
    m_bSupportsStandalone(bSupportsStandalone),
    m_bSupportsKeyboard(bSupportsKeyboard)
{
  m_extensions.insert(extensions.begin(), extensions.end());

  InitializeProperties();
}

void CGameClient::InitializeProperties(void)
{
  m_bIsPlaying = false;
  m_demuxer = nullptr;
  m_region = GAME_REGION_NTSC;
  m_frameRate = 0.0;
  m_sampleRate = 0.0;
  m_bHasVideo = false;
  m_bHasRawVideo = false;
  m_bHasAudio = false;
  m_bSerializationInited = false;
  m_serializeSize = 0;
  m_bRewindEnabled = false;
}

CGameClient::~CGameClient(void)
{
  /* TODO
  if (m_bIsPlaying && m_demuxer)
    m_player->CloseFile();
  */

  FlushDemux();
}

bool CGameClient::Initialize(void)
{
  // Ensure user profile directory exists for add-on
  if (!CDirectory::Exists(Profile()))
    CDirectory::Create(Profile());

  m_libraryProps.InitializeProperties();

  return Create() == ADDON_STATUS_OK;
}

bool CGameClient::IsType(ADDON::TYPE type) const
{
  return type == ADDON::ADDON_GAMEDLL ||
         type == ADDON::ADDON_GAME;
}

std::string CGameClient::LibPath() const
{
  // If the game client requires a proxy, load its DLL instead
  if (m_pInfo->proxy_dll_count > 0)
    return m_pInfo->proxy_dll_paths[0];

  return CAddon::LibPath();
}

AddonPtr CGameClient::GetRunningInstance() const
{
  GameClientPtr gameAddon;
  if (CGameManager::GetInstance().GetAddonInstance(ID(), gameAddon))
    return std::static_pointer_cast<CAddon>(gameAddon);

  return CAddon::GetRunningInstance();
}

void CGameClient::OnEnabled()
{
  //CGameManager::GetInstance().Enable(ID()); // TODO
}

void CGameClient::OnDisabled()
{
  //CGameManager::GetInstance().Disable(ID()); // TODO
}

bool CGameClient::CanOpen(const CFileItem& file) const
{
  // Try to resolve path to a local file, as not all game clients support VFS
  CURL translatedUrl(CSpecialProtocol::TranslatePath(file.GetPath()));
  if (translatedUrl.GetProtocol() == "file")
    translatedUrl.SetProtocol("");

  // Filter by vfs support
  const bool bIsLocalFS = translatedUrl.GetProtocol().empty();
  if (!bIsLocalFS && !SupportsVFS())
    return false;

  // Directories not currently supported
  if (file.m_bIsFolder)
    return false;

  // Filter by extension
  if (!IsExtensionValid(URIUtils::GetExtension(translatedUrl.Get())))
    return false;

  return true;
}

bool CGameClient::OpenFile(const CFileItem& file, IGameDemuxCallback* demuxer)
{
  if (!demuxer)
    return false;

  CSingleLock lock(m_critSection);

  if (!Initialized())
    return false;

  CloseFile();

  GAME_ERROR error = GAME_ERROR_FAILED;
  std::string strFilePath;

  if (m_bSupportsStandalone)
  {
    CLog::Log(LOGDEBUG, "GameClient: Loading %s in standalone mode", ID().c_str());

    try { LogError(error = m_pStruct->LoadStandalone(), "LoadStandalone()"); }
    catch (...) { LogException("LoadStandalone()"); }
  }
  else
  {
    CLog::Log(LOGDEBUG, "GameClient: Loading %s", file.GetPath().c_str());

    try { LogError(error = m_pStruct->LoadGame(file.GetPath().c_str()), "LoadGame()"); }
    catch (...) { LogException("LoadGame()"); }
  }

  // If gameplay failed, check for missing optional resources
  if (error != GAME_ERROR_NO_ERROR)
  {
    const ADDONDEPS& dependencies = GetDeps();
    for (ADDONDEPS::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
    {
      const bool bOptional = it->second.second;
      if (bOptional)
      {
        const std::string& strAddonId = it->first;
        AddonPtr dummy;
        const bool bInstalled = CAddonMgr::GetInstance().GetAddon(strAddonId, dummy);
        if (!bInstalled)
        {
          // Failed to play game
          // The add-on %s is missing.
          CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(35209), StringUtils::Format(g_localizeStrings.Get(35210).c_str(), strAddonId.c_str()));
          break;
        }
      }
    }
  }

  if (error == GAME_ERROR_NO_ERROR && LoadGameInfo())
  {
    m_filePath        = file.GetPath();
    m_demuxer         = demuxer;
    m_inputRateHandle = PERIPHERALS::g_peripherals.SetEventScanRate(m_frameRate);
    m_bIsPlaying      = true;

    if (m_bSupportsKeyboard)
      OpenKeyboard();

    return true;
  }

  return false;
}

bool CGameClient::LoadGameInfo()
{
  // Get information about system audio/video timings and geometry
  // Can be called only after retro_load_game()
  game_system_av_info av_info = { };

  GAME_ERROR error = GAME_ERROR_FAILED;
  try { LogError(error = m_pStruct->GetGameInfo(&av_info), "GetGameInfo()"); }
  catch (...) { LogException("GetGameInfo()"); }

  if (error != GAME_ERROR_NO_ERROR)
    return false;

  GAME_REGION region;
  try { region = m_pStruct->GetRegion(); }
  catch (...) { LogException("GetRegion()"); return false; }

  CLog::Log(LOGINFO, "GAME: ---------------------------------------");
  CLog::Log(LOGINFO, "GAME: Opened file %s",   m_filePath.c_str());
  CLog::Log(LOGINFO, "GAME: Base Width:   %u", av_info.geometry.base_width);
  CLog::Log(LOGINFO, "GAME: Base Height:  %u", av_info.geometry.base_height);
  CLog::Log(LOGINFO, "GAME: Max Width:    %u", av_info.geometry.max_width);
  CLog::Log(LOGINFO, "GAME: Max Height:   %u", av_info.geometry.max_height);
  CLog::Log(LOGINFO, "GAME: Aspect Ratio: %f", av_info.geometry.aspect_ratio);
  CLog::Log(LOGINFO, "GAME: FPS:          %f", av_info.timing.fps);
  CLog::Log(LOGINFO, "GAME: Sample Rate:  %f", av_info.timing.sample_rate);
  CLog::Log(LOGINFO, "GAME: Region:       %s", region == GAME_REGION_NTSC ? GAME_REGION_NTSC_STRING : GAME_REGION_PAL_STRING);
  CLog::Log(LOGINFO, "GAME: ---------------------------------------");

  m_frameRate  = av_info.timing.fps;
  m_sampleRate = av_info.timing.sample_rate;
  m_region     = region;

  return true;
}

bool CGameClient::InitSerialization()
{
  // Check if serialization is supported so savestates and rewind can be used
  unsigned int serializeSize;
  try { serializeSize = m_pStruct->SerializeSize(); }
  catch (...) { LogException("SerializeSize()"); return false; }

  if (serializeSize == 0)
  {
    CLog::Log(LOGINFO, "GAME: Serialization not supported, continuing without save or rewind");
    return false;
  }

  m_serializeSize = serializeSize;
  m_bRewindEnabled = CSettings::GetInstance().GetBool("gamesgeneral.enablerewind");

  // Set up rewind functionality
  if (m_bRewindEnabled)
  {
    m_serialState.Init(m_serializeSize, (size_t)(CSettings::GetInstance().GetInt("gamesgeneral.rewindtime") * m_frameRate));

    GAME_ERROR error = GAME_ERROR_FAILED;
    try { LogError(error = m_pStruct->Serialize(m_serialState.GetState(), m_serialState.GetFrameSize()), "Serialize()"); }
    catch (...) { LogException("Serialize()"); }

    if (error != GAME_ERROR_NO_ERROR)
    {
      m_serializeSize = 0;
      m_bRewindEnabled = false;
      m_serialState.Reset();
      CLog::Log(LOGERROR, "GAME: Unable to serialize state, proceeding without save or rewind");
      return false;
    }
  }

  m_bSerializationInited = true;

  return true;
}

void CGameClient::Reset()
{
  if (m_bIsPlaying)
  {
    try { LogError(m_pStruct->Reset(), "Reset()"); }
    catch (...) { LogException("Reset()"); }

    if (m_bRewindEnabled)
    {
      m_serialState.ReInit();

      GAME_ERROR error = GAME_ERROR_FAILED;
      try { LogError(error = m_pStruct->Serialize(m_serialState.GetNextState(), m_serialState.GetFrameSize()), "Serialize()"); }
      catch (...) { LogException("Serialize()"); }

      if (error != GAME_ERROR_NO_ERROR)
        m_bRewindEnabled = false;
    }
  }
}

void CGameClient::CloseFile()
{
  CSingleLock lock(m_critSection);

  if (Initialized() && m_bIsPlaying)
  {
    try { LogError(m_pStruct->UnloadGame(), "UnloadGame()"); }
    catch (...) { LogException("UnloadGame()"); }
  }

  ClearPorts();

  if (m_bSupportsKeyboard)
    CloseKeyboard();

  m_bIsPlaying = false;
  m_filePath.clear();
  if (m_inputRateHandle)
  {
    m_inputRateHandle->Release();
    m_inputRateHandle.reset();
  }
  m_demuxer = NULL;

  FlushDemux();
}

bool CGameClient::OpenVideoStream(AVCodecID codec, AVPixelFormat pixelFormat, unsigned int width, unsigned int height)
{
  if (m_demuxer && !m_bHasVideo)
  {
    m_demuxer->OpenVideoStream(codec, pixelFormat, width, height);
    m_bHasVideo = true;
    m_bHasRawVideo = (codec == AV_CODEC_ID_NONE);
    return true;
  }
  return false;
}

void CGameClient::AddVideoData(const uint8_t* data, unsigned int size)
{
  if (m_bHasVideo)
  {
    DemuxPacket* packet = CDVDDemuxUtils::AllocateDemuxPacket(size);
    if (packet)
    {
      std::memcpy(packet->pData, data, size);

      packet->iSize = size;
      packet->iStreamId = GAME_STREAM_VIDEO_ID;

      {
        CSingleLock lock(m_demuxMutex);

        if (m_bHasRawVideo)
        {
          // Free stale frames
          m_buffer.erase(std::remove_if(m_buffer.begin(), m_buffer.end(),
            [](DemuxPacket* packet)
            {
              if (packet->iStreamId == GAME_STREAM_VIDEO_ID)
              {
                CDVDDemuxUtils::FreeDemuxPacket(packet);
                return true;
              }
              return false;
            }), m_buffer.end());
        }

        m_buffer.push_back(packet);
      }

      m_demuxEvent.Set();
    }
  }
}

void CGameClient::CloseVideoStream()
{
  if (m_demuxer && m_bHasVideo)
    m_demuxer->CloseVideoStream();
  m_bHasVideo = false;
}

bool CGameClient::OpenAudioStream(AVCodecID codec, unsigned int samplerate, const CAEChannelInfo& channelLayout)
{
  if (m_demuxer && !m_bHasAudio)
  {
    m_demuxer->OpenAudioStream(codec, samplerate, channelLayout);
    m_bHasAudio = true;
    return true;
  }
  return false;
}

void CGameClient::AddAudioData(const uint8_t* data, unsigned int size)
{
  if (m_bHasAudio)
  {
    DemuxPacket* packet = CDVDDemuxUtils::AllocateDemuxPacket(size);
    if (packet)
    {
      std::memcpy(packet->pData, data, size);

      packet->iSize = size;
      packet->iStreamId = GAME_STREAM_AUDIO_ID;

      {
        CSingleLock lock(m_demuxMutex);
        m_buffer.push_back(packet);
      }

      m_demuxEvent.Set();
    }
  }
}

void CGameClient::CloseAudioStream()
{
  if (m_demuxer && m_bHasAudio)
    m_demuxer->CloseAudioStream();
  m_bHasAudio = false;
}

DemuxPacket* CGameClient::ReadDemux()
{
  DemuxPacket* packet = nullptr;

  bool bHasPacket;

  {
    CSingleLock lock(m_demuxMutex);
    bHasPacket = !m_buffer.empty();
  }

  if (!bHasPacket)
    bHasPacket = m_demuxEvent.WaitMSec(DEMUX_READ_TIMEOUT_MS);

  if (bHasPacket)
  {
    CSingleLock lock(m_demuxMutex);
    if (!m_buffer.empty())
    {
      packet = m_buffer[0];
      m_buffer.erase(m_buffer.begin());
    }
  }

  return packet;
}

void CGameClient::FlushDemux()
{
  CSingleLock lock(m_demuxMutex);

  for (DemuxPacket* packet : m_buffer)
    CDVDDemuxUtils::FreeDemuxPacket(packet);

  m_buffer.clear();

  m_demuxEvent.Set();
}

unsigned int CGameClient::RewindFrames(unsigned int frames)
{
  CSingleLock lock(m_critSection);

  unsigned int rewound = 0;
  if (m_bIsPlaying && m_bRewindEnabled)
  {
    rewound = m_serialState.RewindFrames(frames);
    if (rewound != 0)
    {
      try { LogError(m_pStruct->Deserialize(m_serialState.GetState(), m_serialState.GetFrameSize()), "Deserialize()"); }
      catch (...) { LogException("Deserialize()"); }
    }
  }
  return rewound;
}

bool CGameClient::OpenPort(unsigned int port)
{
  // Fail if port is already open
  if (port < m_controllers.size() && m_controllers[port] != nullptr)
    return false;

  ControllerVector controllers = GetControllers();
  if (!controllers.empty())
  {
    // TODO: Choose controller
    ControllerPtr& controller = controllers[0];

    if (controller->LoadLayout())
    {
      ClosePort(port);

      // Ensure port exists
      if (port >= m_controllers.size())
        m_controllers.resize(port + 1);

      m_controllers[port] = new CGameClientInput(this, port, controller);

      // If keyboard input is being captured by this add-on, force the port type to PERIPHERAL_JOYSTICK
      PERIPHERALS::PeripheralType device = PERIPHERALS::PERIPHERAL_UNKNOWN;
      if (m_bSupportsKeyboard)
        device = PERIPHERALS::PERIPHERAL_JOYSTICK;

      CPortManager::GetInstance().OpenPort(m_controllers[port], port, device);

      UpdatePort(port, controller);

      return true;
    }
  }

  return false;
}

void CGameClient::ClosePort(unsigned int port)
{
  // Can't close port if it doesn't exist
  if (port >= m_controllers.size())
    return;

  if (m_controllers[port] != NULL)
  {
    CPortManager::GetInstance().ClosePort(m_controllers[port]);

    delete m_controllers[port];
    m_controllers[port] = NULL;

    UpdatePort(port, CController::EmptyPtr);
  }
}

void CGameClient::UpdatePort(unsigned int port, const ControllerPtr& controller)
{
  if (controller != CController::EmptyPtr)
  {
    std::string strId = controller->ID();

    game_controller controllerStruct;

    controllerStruct.controller_id        = strId.c_str();
    controllerStruct.digital_button_count = controller->Layout().FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::DIGITAL);
    controllerStruct.analog_button_count  = controller->Layout().FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::ANALOG);
    controllerStruct.analog_stick_count   = controller->Layout().FeatureCount(FEATURE_TYPE::ANALOG_STICK);
    controllerStruct.accelerometer_count  = controller->Layout().FeatureCount(FEATURE_TYPE::ACCELEROMETER);
    controllerStruct.key_count            = 0; // TODO
    controllerStruct.rel_pointer_count    = 0; // TODO
    controllerStruct.abs_pointer_count    = 0; // TODO

    try { m_pStruct->UpdatePort(port, true, &controllerStruct); }
    catch (...) { LogException("UpdatePort()"); }
  }
  else
  {
    try { m_pStruct->UpdatePort(port, false, NULL); }
    catch (...) { LogException("UpdatePort()"); }
  }
}

bool CGameClient::HasFeature(const std::string& controller, const std::string& feature)
{
  try { return m_pStruct->HasFeature(controller.c_str(), feature.c_str()); }
  catch (...) { LogException("HasFeature()"); }

  return false;
}

void CGameClient::ClearPorts(void)
{
  for (unsigned int i = 0; i < m_controllers.size(); i++)
    ClosePort(i);
}

void CGameClient::OpenKeyboard(void)
{
  CInputManager::GetInstance().RegisterKeyboardHandler(this);
}

void CGameClient::CloseKeyboard(void)
{
  CInputManager::GetInstance().UnregisterKeyboardHandler(this);
}

ControllerVector CGameClient::GetControllers(void) const
{
  ControllerVector controllers;

  const ADDONDEPS& dependencies = GetDeps();
  for (ADDONDEPS::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
  {
    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(it->first, addon, ADDON_GAME_CONTROLLER))
    {
      ControllerPtr controller = std::dynamic_pointer_cast<CController>(addon);
      if (controller)
        controllers.push_back(controller);
    }
  }

  if (controllers.empty())
  {
    // Use the default controller
    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(DEFAULT_CONTROLLER_ID, addon, ADDON_GAME_CONTROLLER))
      controllers.push_back(std::static_pointer_cast<CController>(addon));
  }

  return controllers;
}

bool CGameClient::OnButtonPress(int port, const std::string& feature, bool bPressed)
{
  // Only allow activated input in fullscreen game
  if (bPressed && g_windowManager.GetActiveWindowID() != WINDOW_FULLSCREEN_VIDEO)
  {
    CLog::Log(LOGDEBUG, "GameClient: button press ignored, not in fullscreen game");
    return false;
  }

  std::string strControllerId = m_controllers[port]->ControllerID();

  bool bHandled = false;

  game_input_event event;

  event.type                   = GAME_INPUT_EVENT_DIGITAL_BUTTON;
  event.port                   = port;
  event.controller_id          = strControllerId.c_str();
  event.feature_name           = feature.c_str();
  event.digital_button.pressed = bPressed;

  try { bHandled = m_pStruct->InputEvent(port, &event); }
  catch (...) { LogException("InputEvent()"); }

  return bHandled;
}

bool CGameClient::OnButtonMotion(int port, const std::string& feature, float magnitude)
{
  // Only allow activated input in fullscreen game
  if (magnitude && g_windowManager.GetActiveWindowID() != WINDOW_FULLSCREEN_VIDEO)
    return false;

  std::string strControllerId = m_controllers[port]->ControllerID();

  bool bHandled = false;

  game_input_event event;

  event.type                    = GAME_INPUT_EVENT_ANALOG_BUTTON;
  event.port                    = port;
  event.controller_id           = strControllerId.c_str();
  event.feature_name            = feature.c_str();
  event.analog_button.magnitude = magnitude;

  try { bHandled = m_pStruct->InputEvent(port, &event); }
  catch (...) { LogException("InputEvent()"); }

  return bHandled;
}

bool CGameClient::OnAnalogStickMotion(int port, const std::string& feature, float x, float y)
{
  // Only allow activated input in fullscreen game
  if ((x || y) && g_windowManager.GetActiveWindowID() != WINDOW_FULLSCREEN_VIDEO)
    return false;

  std::string strControllerId = m_controllers[port]->ControllerID();

  bool bHandled = false;

  game_input_event event;

  event.type           = GAME_INPUT_EVENT_ANALOG_STICK;
  event.port           = port;
  event.controller_id  = strControllerId.c_str();
  event.feature_name   = feature.c_str();
  event.analog_stick.x = x;
  event.analog_stick.y = y;

  try { bHandled = m_pStruct->InputEvent(port, &event); }
  catch (...) { LogException("InputEvent()"); }

  return bHandled;
}

bool CGameClient::OnAccelerometerMotion(int port, const std::string& feature, float x, float y, float z)
{
  // Only allow activated input in fullscreen game
  if ((x || y || z) && g_windowManager.GetActiveWindowID() != WINDOW_FULLSCREEN_VIDEO)
    return false;

  std::string strControllerId = m_controllers[port]->ControllerID();

  bool bHandled = false;

  game_input_event event;

  event.type            = GAME_INPUT_EVENT_ACCELEROMETER;
  event.port            = port;
  event.controller_id   = strControllerId.c_str();
  event.feature_name    = feature.c_str();
  event.accelerometer.x = x;
  event.accelerometer.y = y;
  event.accelerometer.z = z;

  try { bHandled = m_pStruct->InputEvent(port, &event); }
  catch (...) { LogException("InputEvent()"); }

  return bHandled;
}

bool CGameClient::OnKeyPress(const CKey& key)
{
  // Only allow activated input in fullscreen game
  if (g_windowManager.GetActiveWindowID() != WINDOW_FULLSCREEN_VIDEO)
  {
    CLog::Log(LOGDEBUG, "GameClient: key press ignored, not in fullscreen game");
    return false;
  }

  bool bHandled = false;

  game_input_event event;

  event.type            = GAME_INPUT_EVENT_KEY;
  event.port            = 0;
  event.controller_id   = ""; // TODO
  event.feature_name    = ""; // TODO
  event.key.pressed     = true;
  event.key.character   = key.GetButtonCode() & BUTTON_INDEX_MASK;
  event.key.modifiers   = GetModifiers(static_cast<CKey::Modifier>(key.GetModifiers()));

  if (event.key.character != 0)
  {
    try { bHandled = m_pStruct->InputEvent(0, &event); }
    catch (...) { LogException("InputEvent()"); }
  }

  return bHandled;
}

void CGameClient::OnKeyRelease(const CKey& key)
{
  game_input_event event;

  event.type            = GAME_INPUT_EVENT_KEY;
  event.port            = 0;
  event.controller_id   = ""; // TODO
  event.feature_name    = ""; // TODO
  event.key.pressed     = false;
  event.key.character   = key.GetButtonCode() & BUTTON_INDEX_MASK;
  event.key.modifiers   = GetModifiers(static_cast<CKey::Modifier>(key.GetModifiers()));

  if (event.key.character != 0)
  {
    try { m_pStruct->InputEvent(0, &event); }
    catch (...) { LogException("InputEvent()"); }
  }
}

bool CGameClient::IsExtensionValid(const std::string& strExtension) const
{
  if (strExtension.empty())
    return false;

  if (m_extensions.empty())
    return false; // Game client didn't provide any extensions

  if (m_extensions.find(EXTENSION_WILDCARD) != m_extensions.end())
    return true; // Game client accepts all extensions

  return m_extensions.find(NormalizeExtension(strExtension)) != m_extensions.end();
}

std::vector<std::string> CGameClient::ParseExtensions(const std::string& strExtensionList)
{
  std::vector<std::string> extensions = StringUtils::Split(strExtensionList, EXTENSION_SEPARATOR);

  std::transform(extensions.begin(), extensions.end(), extensions.begin(), NormalizeExtension);

  return extensions;
}

/*
void CGameClient::SetPlatforms(const string& strPlatformList)
{
  m_platforms.clear();

  vector<string> platforms = StringUtils::Split(strPlatformList, EXTENSION_SEPARATOR);
  for (vector<string>::iterator it = platforms.begin(); it != platforms.end(); it++)
  {
    StringUtils::Trim(*it);
    GamePlatform id = CGameInfoTagLoader::GetPlatformInfoByName(*it).id;
    if (id != PLATFORM_UNKNOWN)
      m_platforms.insert(id);
  }
}
*/

GAME_KEY_MOD CGameClient::GetModifiers(CKey::Modifier modifier)
{
  unsigned int mods = GAME_KEY_MOD_NONE;

  if (modifier & CKey::MODIFIER_CTRL)  mods = mods | GAME_KEY_MOD_CTRL;
  if (modifier & CKey::MODIFIER_SHIFT) mods = mods | GAME_KEY_MOD_SHIFT;
  if (modifier & CKey::MODIFIER_ALT)   mods = mods | GAME_KEY_MOD_ALT;
  if (modifier & CKey::MODIFIER_RALT)  mods = mods | GAME_KEY_MOD_RALT;
  if (modifier & CKey::MODIFIER_META)  mods = mods | GAME_KEY_MOD_META;

  return static_cast<GAME_KEY_MOD>(mods);
}

bool CGameClient::LogError(GAME_ERROR error, const char* strMethod) const
{
  if (error != GAME_ERROR_NO_ERROR)
  {
    CLog::Log(LOGERROR, "GAME - %s - addon '%s' returned an error: %s",
        strMethod, ID().c_str(), ToString(error));
    return false;
  }
  return true;
}

void CGameClient::LogException(const char* strFunctionName) const
{
  CLog::Log(LOGERROR, "GAME: exception caught while trying to call '%s' on add-on %s",
      strFunctionName, ID().c_str());
  CLog::Log(LOGERROR, "Please contact the developer of this add-on: %s", Author().c_str());
}

void CGameClient::LogAddonProperties(void) const
{
  std::vector<std::string> vecExtensions(m_extensions.begin(), m_extensions.end());

  CLog::Log(LOGINFO, "GAME: ------------------------------------");
  CLog::Log(LOGINFO, "GAME: Loaded DLL for %s", ID().c_str());
  CLog::Log(LOGINFO, "GAME: Client: %s at version %s", Name().c_str(), Version().asString().c_str());
  CLog::Log(LOGINFO, "GAME: Valid extensions: %s", StringUtils::Join(vecExtensions, " ").c_str());
  CLog::Log(LOGINFO, "GAME: Supports VFS: %s", m_bSupportsVFS ? "yes" : "no");
  CLog::Log(LOGINFO, "GAME: Supports standalone execution: %s", m_bSupportsStandalone ? "yes" : "no");
  CLog::Log(LOGINFO, "GAME: ------------------------------------");
}

const char* CGameClient::ToString(GAME_ERROR error)
{
  switch (error)
  {
    case GAME_ERROR_NO_ERROR:
      return "no error";
    case GAME_ERROR_NOT_IMPLEMENTED:
      return "not implemented";
    case GAME_ERROR_REJECTED:
      return "rejected by the client";
    case GAME_ERROR_INVALID_PARAMETERS:
      return "invalid parameters for this method";
    case GAME_ERROR_FAILED:
      return "the command failed";
    case GAME_ERROR_UNKNOWN:
    default:
      return "unknown error";
  }
}
