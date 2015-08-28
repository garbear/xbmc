/*
 *      Copyright (C) 2012-2015 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameClient.h"
#include "addons/AddonManager.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "games/GameManager.h"
#include "input/InputManager.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/PortManager.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <assert.h>

using namespace ADDON;
using namespace GAME;
using namespace XFILE;

#define EXTENSION_SEPARATOR          "|"
#define EXTENSION_WILDCARD           "*"
#define GAME_REGION_NTSC_STRING      "NTSC"
#define GAME_REGION_PAL_STRING       "PAL"
#define MAX_LAUNCH_FILE_CHOICES      20 // Show up to this many games when a direcory is played

// --- NormalizeExtension ------------------------------------------------------

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

// --- CControllerInput ------------------------------------------------------------

CControllerInput::CControllerInput(CGameClient* addon, int port, const GameControllerPtr& controller)
  : m_addon(addon),
    m_port(port),
    m_controller(controller)
{
  assert(m_addon != NULL);
  assert(controller.get() != NULL);
}

std::string CControllerInput::ControllerID(void) const
{
  return m_controller->ID();
}

InputType CControllerInput::GetInputType(const std::string& feature) const
{
  const std::vector<CGameControllerFeature>& features = m_controller->Layout().Features();

  for (std::vector<CGameControllerFeature>::const_iterator it = features.begin(); it != features.end(); ++it)
  {
    if (feature == it->Name())
    {
      switch (it->Type())
      {
        case FEATURE_BUTTON:
          if (it->ButtonType() == BUTTON_DIGITAL)
            return INPUT_TYPE_DIGITAL;
          else if (it->ButtonType() == BUTTON_ANALOG)
            return INPUT_TYPE_ANALOG;
          break;

        default:
          break;
      }
      break;
    }
  }

  return INPUT_TYPE_UNKNOWN;
}

bool CControllerInput::OnButtonPress(const std::string& feature, bool bPressed)
{
  return m_addon->OnButtonPress(m_port, feature, bPressed);
}

bool CControllerInput::OnButtonMotion(const std::string& feature, float magnitude)
{
  return m_addon->OnButtonMotion(m_port, feature, magnitude);
}

bool CControllerInput::OnAnalogStickMotion(const std::string& feature, float x, float y)
{
  return m_addon->OnAnalogStickMotion(m_port, feature, x, y);
}

bool CControllerInput::OnAccelerometerMotion(const std::string& feature, float x, float y, float z)
{
  return m_addon->OnAccelerometerMotion(m_port, feature, x, y, z);
}

// --- CGameClient -------------------------------------------------------------

CGameClient::CGameClient(const AddonProps& props)
  : CAddonDll<DllGameClient, GameClient, game_client_properties>(props),
    m_apiVersion("0.0.0"),
    m_libraryProps(this, m_pInfo)
{
  InitializeProperties();

  InfoMap::const_iterator it;
  /*
  if ((it = props.extrainfo.find("platforms")) != props.extrainfo.end())
    SetPlatforms(it->second);
  */
  if ((it = props.extrainfo.find("extensions")) != props.extrainfo.end())
  {
    std::vector<std::string> extensions = ParseExtensions(it->second);
    m_extensions.insert(extensions.begin(), extensions.end());
  }
  if ((it = props.extrainfo.find("supports_vfs")) != props.extrainfo.end())
    m_bSupportsVFS = (it->second == "true" || it->second == "yes");
  if ((it = props.extrainfo.find("supports_no_game")) != props.extrainfo.end())
    m_bSupportsStandalone = (it->second == "true" || it->second == "yes");
  if ((it = props.extrainfo.find("supports_keyboard")) != props.extrainfo.end())
    m_bSupportsKeyboard = (it->second == "true" || it->second == "yes");
}

CGameClient::CGameClient(const cp_extension_t* ext)
  : CAddonDll<DllGameClient, GameClient, game_client_properties>(ext),
    m_apiVersion("0.0.0"),
    m_libraryProps(this, m_pInfo)
{
  InitializeProperties();

  if (ext)
  {
    /*
    string strPlatforms = CAddonMgr::Get().GetExtValue(ext->configuration, "platforms");
    if (!strPlatforms.empty())
    {
      Props().extrainfo.insert(make_pair("platforms", strPlatforms));
      SetPlatforms(strPlatforms);
    }
    */
    std::string strExtensions = CAddonMgr::Get().GetExtValue(ext->configuration, "extensions");
    if (!strExtensions.empty())
    {
      Props().extrainfo.insert(make_pair("extensions", strExtensions));
      std::vector<std::string> extensions = ParseExtensions(strExtensions);
      m_extensions.insert(extensions.begin(), extensions.end());
    }
    std::string strSupportsVFS = CAddonMgr::Get().GetExtValue(ext->configuration, "supports_vfs");
    if (!strSupportsVFS.empty())
    {
      Props().extrainfo.insert(make_pair("supports_vfs", strSupportsVFS));
      m_bSupportsVFS = (strSupportsVFS == "true" || strSupportsVFS == "yes");
    }
    std::string strSupportsStandalone = CAddonMgr::Get().GetExtValue(ext->configuration, "supports_no_game");
    if (!strSupportsStandalone.empty())
    {
      Props().extrainfo.insert(make_pair("supports_no_game", strSupportsStandalone));
      m_bSupportsStandalone = (strSupportsStandalone == "true" || strSupportsStandalone == "yes");
    }
    std::string strSupportsKeyboard = CAddonMgr::Get().GetExtValue(ext->configuration, "supports_keyboard");
    if (!strSupportsKeyboard.empty())
    {
      Props().extrainfo.insert(make_pair("supports_keyboard", strSupportsKeyboard));
      m_bSupportsKeyboard = (strSupportsKeyboard == "true" || strSupportsKeyboard == "yes");
    }
  }
}

void CGameClient::InitializeProperties(void)
{
  m_bSupportsVFS = false;
  m_bSupportsStandalone = false;
  m_bSupportsKeyboard = false;
  m_bIsPlaying = false;
  m_player = NULL;
  m_region = GAME_REGION_NTSC;
  m_frameRate = 0.0;
  m_frameRateCorrection = 1.0;
  m_sampleRate = 0.0;
  m_serializeSize = 0;
  m_bRewindEnabled = false;
}

CGameClient::~CGameClient(void)
{
  if (m_bIsPlaying && m_player)
    m_player->CloseFile();
}

bool CGameClient::Initialize(void)
{
  m_libraryProps.InitializeProperties();
  return Create() == ADDON_STATUS_OK;
}

const std::string CGameClient::LibPath() const
{
  // If the game client requires a proxy, load its DLL instead
  if (m_pInfo->proxy_dll_count > 0)
    return m_pInfo->proxy_dll_paths[0];

  return CAddon::LibPath();
}

AddonPtr CGameClient::GetRunningInstance() const
{
  GameClientPtr gameAddon;
  if (CGameManager::Get().GetClient(ID(), gameAddon))
    return std::dynamic_pointer_cast<CAddon>(gameAddon);

  return CAddon::GetRunningInstance();
}

void CGameClient::OnEnabled()
{
  CGameManager::Get().RegisterAddon(std::dynamic_pointer_cast<CGameClient>(GetRunningInstance()));
}

void CGameClient::OnDisabled()
{
  CGameManager::Get().UnregisterAddonByID(ID());
}

bool CGameClient::CanOpen(const CFileItem& file) const
{
  // Filter by Addon.ID property
  if (file.HasProperty("Addon.ID") && file.GetProperty("Addon.ID").asString() != ID())
    return false;

  // Try to resolve path to a local file, as not all game clients support VFS
  CURL translatedUrl(CSpecialProtocol::TranslatePath(file.GetPath()));
  if (translatedUrl.GetProtocol() == "file")
    translatedUrl.SetProtocol("");

  // Filter by vfs support
  const bool bIsLocalFS = translatedUrl.GetProtocol().empty();
  if (!bIsLocalFS && !SupportsVFS())
    return false;

  if (file.m_bIsFolder)
    return CanOpenDirectory(translatedUrl.Get());
  else
    return CanOpenFile(translatedUrl.Get());
}

bool CGameClient::CanOpenFile(std::string strPath) const
{
  // Filter by extension
  if (IsExtensionValid(URIUtils::GetExtension(strPath)))
    return true;

  if (URIUtils::IsArchive(strPath))
    return CanOpenDirectory(strPath);

  return false;
}

bool CGameClient::CanOpenDirectory(std::string strPath) const
{
  // Turn archive files into archive paths
  if (URIUtils::IsArchive(strPath))
  {
    // Must support VFS to load by zip:// protocol
    if (!SupportsVFS())
      return false;

    strPath = ToArchivePath(strPath);
  }

  CFileItemList itemList;
  if (ResolveDirectory(strPath, itemList))
    return !itemList.IsEmpty();

  return false;
}

std::string CGameClient::ResolvePath(const CFileItem& file) const
{
  CURL resolvedUrl(CSpecialProtocol::TranslatePath(file.GetPath()));
  if (resolvedUrl.GetProtocol() == "file")
    resolvedUrl.SetProtocol("");

  std::string strResolvedPath;

  std::string strPath = resolvedUrl.Get();

  const bool bIsValidExt = !file.m_bIsFolder && IsExtensionValid(URIUtils::GetExtension(strPath));
  if (bIsValidExt)
  {
    strResolvedPath = strPath;
  }
  else
  {
    bool bIsFolder = file.m_bIsFolder;

    if (URIUtils::IsArchive(strPath))
    {
      // Turn archive files into archive paths
      strPath = ToArchivePath(strPath);
      bIsFolder |= !strPath.empty();
    }

    if (bIsFolder)
    {
      CFileItemList files;
      if (ResolveDirectory(strPath, files))
      {
        if (files.Size() == 1)
        {
          strResolvedPath = files[0]->GetPath();
        }
        else if (files.Size() >= 2)
        {
          CLog::Log(LOGDEBUG, "%s: Directory contains %d possible game files (%s)", ID().c_str(), files.Size(), strPath.c_str());

          // Don't show too many files
          unsigned int fileCount = std::min((unsigned int)files.Size(), (unsigned int)MAX_LAUNCH_FILE_CHOICES);

          CContextButtons choices;
          choices.reserve(fileCount);

          for (unsigned int i = 0; i < fileCount; i++)
            choices.push_back(std::make_pair(i, files[i]->GetLabel()));

          int result = CGUIDialogContextMenu::ShowAndGetChoice(choices);
          if (result >= 0)
            strResolvedPath = files[result]->GetPath();
          else
            CLog::Log(LOGDEBUG, "%s: User cancelled game file selection", ID().c_str());
        }

        if (!strResolvedPath.empty())
          CLog::Log(LOGDEBUG, "%s: Directory resolved to file: %s", ID().c_str(), strResolvedPath.c_str());
      }
    }
  }

  return strResolvedPath;
}

std::string CGameClient::ToArchivePath(const std::string& strPath)
{
  std::string strArchivePath;

  std::string strType = URIUtils::GetExtension(strPath);
  if (!strType.empty())
  {
    if (strType[0] == '.')
      strType.erase(strType.begin());
    strArchivePath = URIUtils::CreateArchivePath(strType, CURL(strPath)).Get();
  }

  return strArchivePath;
}

bool CGameClient::ResolveDirectory(const std::string& strPath, CFileItemList& files) const
{
  CFileItemList directories;
  if (GetDirectory(strPath, files, directories))
  {
    if (files.IsEmpty() && directories.Size() == 1)
    {
      // Try again in subdirectory if there are no files and a single directory
      CFileItemList dummy;
      GetDirectory(directories[0]->GetPath(), files, dummy);
    }
  }

  return !files.IsEmpty();
}

bool CGameClient::GetDirectory(std::string strPath, CFileItemList& files, CFileItemList& directories) const
{
  if (m_extensions.empty())
    return false;

  std::string strValidExts;
  if (!IsExtensionValid(EXTENSION_WILDCARD))
  {
    for (std::set<std::string>::const_iterator it = m_extensions.begin(); it != m_extensions.end(); it++)
      strValidExts += *it + "|";
  }

  CFileItemList items;
  if (CDirectory::GetDirectory(strPath, items, strValidExts, DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO))
  {
    for (int i = 0; i < items.Size(); i++)
    {
      if (items[i]->m_bIsFolder)
        directories.Add(items[i]);
      else
        files.Add(items[i]);
    }
    return true;
  }

  return false;
}

bool CGameClient::OpenFile(const CFileItem& file, IPlayer* player)
{
  // Filter by Addon.ID property
  if (file.HasProperty("Addon.ID") && file.GetProperty("Addon.ID").asString() != ID())
    return false;

  CSingleLock lock(m_critSection);

  if (!Initialized())
    return false;

  if (!m_bSupportsStandalone && !CanOpen(file))
    return false;

  CloseFile();

  GAME_ERROR error = GAME_ERROR_FAILED;
  std::string strFilePath;

  if (m_bSupportsStandalone)
  {
    try { LogError(error = m_pStruct->LoadStandalone(), "LoadStandalone()"); }
    catch (...) { LogException("LoadStandalone()"); }
  }
  else
  {
    std::string strFilePath = ResolvePath(file);
    if (strFilePath.empty())
      return false;

    CLog::Log(LOGDEBUG, "GameClient: Loading %s", strFilePath.c_str());

    try { LogError(error = m_pStruct->LoadGame(strFilePath.c_str()), "LoadGame()"); }
    catch (...) { LogException("LoadGame()"); }
  }

  if (error == GAME_ERROR_NO_ERROR && LoadGameInfo())
  {
    m_filePath   = strFilePath;
    m_player     = player;
    m_bIsPlaying = true;

    InitSerialization();

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
  m_bRewindEnabled = CSettings::Get().GetBool("gamesgeneral.enablerewind");

  // Set up rewind functionality
  if (m_bRewindEnabled)
  {
    m_serialState.Init(m_serializeSize, (size_t)(CSettings::Get().GetInt("gamesgeneral.rewindtime") * GetFrameRate()));

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

  return true;
}

void CGameClient::Reset()
{
  if (m_bIsPlaying)
  {
    // TODO: Reset all controller ports to their same value. bSNES since v073r01
    // resets controllers to JOYPAD after a reset, so guard against this.
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
  m_player = NULL;
}

void CGameClient::RunFrame()
{
  CSingleLock lock(m_critSection);

  if (!m_bIsPlaying)
    return;

  try { m_pStruct->FrameEvent(); }
  catch (...) { LogException("FrameEvent()"); }

  // Append a new state delta to the rewind buffer
  if (m_bRewindEnabled)
  {
	GAME_ERROR error = GAME_ERROR_FAILED;
    try { LogError(error = m_pStruct->Serialize(m_serialState.GetNextState(), m_serialState.GetFrameSize()), "Serialize()"); }
    catch (...) { LogException("Serialize()"); }

    if (error == GAME_ERROR_NO_ERROR)
      m_serialState.AdvanceFrame();
    else
      m_bRewindEnabled = false;
  }
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
  GameControllerVector controllers = GetControllers();
  if (!controllers.empty()) // TODO: Choose controller
  {
    if (port >= m_controllers.size())
      m_controllers.resize(port + 1);

    ClosePort(port);

    m_controllers[port] = new CControllerInput(this, port, controllers[0]);

    CPortManager::Get().OpenPort(m_controllers[port], port);

    UpdatePort(port, controllers[0]);

    return true;
  }

  return false;
}

void CGameClient::ClosePort(unsigned int port)
{
  if (port >= m_controllers.size())
    return;

  if (m_controllers[port] != NULL)
  {
    CPortManager::Get().ClosePort(m_controllers[port]);

    delete m_controllers[port];
    m_controllers[port] = NULL;

    UpdatePort(port, CGameController::EmptyPtr);
  }
}

void CGameClient::ClearPorts(void)
{
  for (unsigned int i = 0; i < m_controllers.size(); i++)
    ClosePort(i);
}

void CGameClient::UpdatePort(unsigned int port, const GameControllerPtr& controller)
{
  if (controller != CGameController::EmptyPtr)
  {
    game_controller controllerStruct;

    const std::string strId = controller->ID();
    controllerStruct.controller_id        = strId.c_str();
    controllerStruct.digital_button_count = controller->Layout().FeatureCount(FEATURE_BUTTON, BUTTON_DIGITAL);
    controllerStruct.analog_button_count  = controller->Layout().FeatureCount(FEATURE_BUTTON, BUTTON_ANALOG);
    controllerStruct.analog_stick_count   = controller->Layout().FeatureCount(FEATURE_ANALOG_STICK);
    controllerStruct.accelerometer_count  = controller->Layout().FeatureCount(FEATURE_ACCELEROMETER);
    controllerStruct.key_count            = controller->Layout().FeatureCount(FEATURE_KEY);
    controllerStruct.rel_pointer_count    = controller->Layout().FeatureCount(FEATURE_RELATIVE_POINTER);
    controllerStruct.abs_pointer_count    = controller->Layout().FeatureCount(FEATURE_ABSOLUTE_POINTER);

    try { m_pStruct->UpdatePort(port, true, &controllerStruct); }
    catch (...) { LogException("UpdatePort()"); }
  }
  else
  {
    try { m_pStruct->UpdatePort(port, false, NULL); }
    catch (...) { LogException("UpdatePort()"); }
  }
}

void CGameClient::OpenKeyboard(void)
{
  CInputManager::Get().RegisterKeyboardHandler(this);
}

void CGameClient::CloseKeyboard(void)
{
  CInputManager::Get().UnregisterKeyboardHandler(this);
}

GameControllerVector CGameClient::GetControllers(void) const
{
  GameControllerVector controllers;

  const ADDONDEPS& dependencies = GetDeps();
  for (ADDONDEPS::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
  {
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(it->first, addon, ADDON_GAME_CONTROLLER))
    {
      GameControllerPtr controller = std::dynamic_pointer_cast<CGameController>(addon);
      if (controller)
        controllers.push_back(controller);
    }
  }

  if (controllers.empty())
  {
    // Use the default controller
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(DEFAULT_GAME_CONTROLLER, addon, ADDON_GAME_CONTROLLER))
      controllers.push_back(std::dynamic_pointer_cast<CGameController>(addon));
  }

  return controllers;
}

bool CGameClient::OnButtonPress(int port, const std::string& feature, bool bPressed)
{
  std::string strControllerId = m_controllers[port]->Controller()->ID();

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
  std::string strControllerId = m_controllers[port]->Controller()->ID();

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
  std::string strControllerId = m_controllers[port]->Controller()->ID();

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
  std::string strControllerId = m_controllers[port]->Controller()->ID();

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
  bool bHandled = false;

  game_input_event event;

  event.type            = GAME_INPUT_EVENT_KEY;
  event.port            = 0;
  event.controller_id   = ""; // TODO
  event.feature_name    = ""; // TODO
  event.key.pressed     = true;
  event.key.character   = key.GetVKey();
  event.key.modifiers   = GetModifiers(static_cast<CKey::Modifier>(key.GetModifiers()));

  CLog::Log(LOGDEBUG, "KEY PRESSED *******");
  CLog::Log(LOGDEBUG, "KEY button code: 0x%08x", key.GetButtonCode());
  CLog::Log(LOGDEBUG, "KEY button code & 0xff: 0x%02x", key.GetButtonCode() & 0xff);
  CLog::Log(LOGDEBUG, "KEY unicode: 0x%08x", key.GetUnicode());
  CLog::Log(LOGDEBUG, "KEY vkey: 0x%02x", key.GetVKey());
  CLog::Log(LOGDEBUG, "KEY modifiers: 0x%04x", key.GetModifiers());

  try { bHandled = m_pStruct->InputEvent(0, &event); }
  catch (...) { LogException("InputEvent()"); }

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
  event.key.character   = key.GetVKey();
  event.key.modifiers   = GetModifiers(static_cast<CKey::Modifier>(key.GetModifiers()));

  CLog::Log(LOGDEBUG, "KEY RELEASED *******");
  CLog::Log(LOGDEBUG, "KEY button code: 0x%08x", key.GetButtonCode());
  CLog::Log(LOGDEBUG, "KEY button code & 0xff: 0x%02x", key.GetButtonCode() & 0xff);
  CLog::Log(LOGDEBUG, "KEY unicode: 0x%08x", key.GetUnicode());
  CLog::Log(LOGDEBUG, "KEY vkey: 0x%02x", key.GetVKey());
  CLog::Log(LOGDEBUG, "KEY modifiers: 0x%04x", key.GetModifiers());

  try { m_pStruct->InputEvent(0, &event); }
  catch (...) { LogException("InputEvent()"); }
}

void CGameClient::SetFrameRateCorrection(double correctionFactor)
{
  if (correctionFactor != 0.0)
    m_frameRateCorrection = correctionFactor;
  if (m_bRewindEnabled)
    m_serialState.SetMaxFrames((size_t)(CSettings::Get().GetInt("gamesgeneral.rewindtime") * GetFrameRate()));
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

GAME_KEY_MOD operator|(GAME_KEY_MOD lhs, GAME_KEY_MOD rhs)
{
  return static_cast<GAME_KEY_MOD>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

GAME_KEY_MOD CGameClient::GetModifiers(CKey::Modifier modifier)
{
  GAME_KEY_MOD mods = GAME_KEY_MOD_NONE;

  if (modifier & CKey::MODIFIER_CTRL)  mods = mods | GAME_KEY_MOD_CTRL;
  if (modifier & CKey::MODIFIER_SHIFT) mods = mods | GAME_KEY_MOD_SHIFT;
  if (modifier & CKey::MODIFIER_ALT)   mods = mods | GAME_KEY_MOD_ALT;
  if (modifier & CKey::MODIFIER_RALT)  mods = mods | GAME_KEY_MOD_ALT;
  if (modifier & CKey::MODIFIER_META)  mods = mods | GAME_KEY_MOD_META;

  return mods;
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
