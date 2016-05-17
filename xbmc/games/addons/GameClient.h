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
#pragma once

/*
 * Adding new functions and callbacks to the XBMC Game API
 *
 * The Game API is spread out across various files. Adding new functions and
 * callbacks is an adventure that spans many layers of abstraction. Necessary
 * steps can easily be omitted, so the process of adding functions and
 * callbacks is documented here.
 *
 * The Game API is layed out in three files:
 *   - kodi_game_dll.h        (API function declarations)
 *   - kodi_game_callbacks.h  (API callback pointers)
 *   - kodi_game_types.h      (API enums and structs)
 *
 * To add a new API function:
 *   1.  Declare the function in kodi_game_dll.h with some helpful documentation
 *   2.  Assign the function pointer in get_addon() of the same file. get_addon()
 *       (aliased to GetAddon()) is called in AddonDll.h immediately after
 *       loading the shared library.
 *   3.  Add the function to the GameClient struct in kodi_game_types.h. This
 *       struct contains pointers to all the API functions. It is populated in
 *       get_addon(). CGameClient invokes API functions through this struct.
 *   4.  Define the function in the cpp file of the game client project
 *
 * To add a new API callback:
 *   1.  Declare the callback as a function pointer in the CB_GameLib struct of
 *       kodi_game_callbacks.h with some helpful documentation. The first
 *       parameter, addonData, is the CAddonCallbacksGame object associated with
 *       the game client instance.
 *   2.  Declare the callback as a static member function of CAddonCallbacksGame
 *   3.  Define the function in AddonCallbacksGame.cpp and assign the callback
 *       to the callbacks table in the constructor.
 *   4.  Expose the function to the game client in libKODI_game.cpp. This shared
 *       library allows for ABI compatibility if the API is unchanged across
 *       releases.
 *   5.  Add the callback to the helper class in libKODI_game.h. Requires three
 *       modifications: register the symbol exported from the shared library,
 *       expose the callback using a member function wrapper, and declare the
 *       function pointer as a protected member variable.
 */

#include "GameClientProperties.h"
#include "GameClientTiming.h"
#include "addons/AddonDll.h"
#include "addons/DllGameClient.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "games/controllers/ControllerTypes.h"
#include "games/GameTypes.h"
//#include "games/SerialState.h"
#include "input/keyboard/IKeyboardHandler.h"
#include "peripherals/EventScanRate.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

class CFileItem;

namespace GAME
{

class CGameClientInput;
class IGameAudioCallback;
class IGameClientPlayback;
class IGameVideoCallback;

// --- CGameClient -------------------------------------------------------------

class CGameClient : public ADDON::CAddonDll<DllGameClient, GameClient, game_client_properties>,
                    public KEYBOARD::IKeyboardHandler
{
public:
  static std::unique_ptr<CGameClient> FromExtension(ADDON::AddonProps props, const cp_extension_t* ext);

  CGameClient(ADDON::AddonProps props);

  virtual ~CGameClient(void);

  // Implementation of IAddon via CAddonDll
  virtual bool            IsType(ADDON::TYPE type) const override;
  virtual std::string     LibPath() const override;
  virtual void            OnDisabled() override;
  virtual void            OnEnabled() override;
  virtual ADDON::AddonPtr GetRunningInstance() const override;

  // Query properties of the game client
  bool                         IsStandalone() const { return m_bSupportsStandalone; }
  bool                         SupportsVFS() const { return m_bSupportsVFS; }
  const std::set<std::string>& GetExtensions() const { return m_extensions; }
  bool                         IsExtensionValid(const std::string& strExtension) const;
  bool                         CanOpen(const CFileItem& file) const;

  // Start/stop gameplay
  bool Initialize(void);
  void Unload();
  bool OpenFile(const CFileItem& file, IGameAudioCallback* audio, IGameVideoCallback* video);
  void Reset();
  void CloseFile();
  const std::string& GetGamePath() const { return m_gamePath; }

  // Playback control
  bool IsPlaying() const { return m_bIsPlaying; }
  IGameClientPlayback* GetPlayback() { return m_playback.get(); }
  const CGameClientTiming& Timing() const { return m_timing; }
  void RunFrame();

  // Audio/video callbacks
  bool OpenPixelStream(GAME_PIXEL_FORMAT format, unsigned int width, unsigned int height);
  bool OpenVideoStream(GAME_VIDEO_CODEC codec);
  bool OpenPCMStream(GAME_PCM_FORMAT format, const GAME_AUDIO_CHANNEL* channelMap);
  bool OpenAudioStream(GAME_AUDIO_CODEC codec, const GAME_AUDIO_CHANNEL* channelMap);
  void AddStreamData(GAME_STREAM_TYPE stream, const uint8_t* data, unsigned int size);
  void CloseStream(GAME_STREAM_TYPE stream);

  // Access memory
  size_t SerializeSize() const { return m_serializeSize; }
  bool Serialize(uint8_t* data, size_t size);
  bool Deserialize(const uint8_t* data, size_t size);

  // Input callbacks
  bool OpenPort(unsigned int port);
  void ClosePort(unsigned int port);
  bool ReceiveInputEvent(const game_input_event& eventStruct);

  // Input functions
  bool HasFeature(const std::string& controller, const std::string& feature);
  bool AcceptsInput(void);
  bool OnButtonPress(int port, const std::string& feature, bool bPressed);
  bool OnButtonMotion(int port, const std::string& feature, float magnitude);
  bool OnAnalogStickMotion(int port, const std::string& feature, float x, float y);
  bool OnAccelerometerMotion(int port, const std::string& feature, float x, float y, float z);

  // implementation of IKeyboardHandler
  virtual bool OnKeyPress(const CKey& key) override;
  virtual void OnKeyRelease(const CKey& key) override;

private:
  // Private gameplay functions
  bool LoadGameInfo(const std::string& logPath);
  bool NormalizeAudio(IGameAudioCallback* audioCallback);
  std::string GetMissingDep();
  void CreatePlayback();
  void ResetPlayback();

  // Private input functions
  void UpdatePort(unsigned int port, const ControllerPtr& controller);
  void ClearPorts(void);
  bool SetRumble(unsigned int port, const std::string& feature, float magnitude);
  void OpenKeyboard(void);
  void CloseKeyboard(void);
  ControllerVector GetControllers(void) const;

  // Private memory stream functions
  size_t GetSerializeSize();

  // Helper functions
  void LogAddonProperties(void) const;
  bool LogError(GAME_ERROR error, const char* strMethod) const;
  void LogException(const char* strFunctionName) const;

  // Add-on properties
  ADDON::AddonVersion   m_apiVersion;
  CGameClientProperties m_libraryProps;        // Properties to pass to the DLL

  // Game API xml parameters
  bool                  m_bSupportsVFS;
  bool                  m_bSupportsGameLoop;
  bool                  m_bSupportsStandalone;
  bool                  m_bSupportsKeyboard;
  std::set<std::string> m_extensions;
  //GamePlatforms         m_platforms;

  // Properties of the current playing file
  std::atomic_bool      m_bIsPlaying;          // True between OpenFile() and CloseFile()
  std::string           m_gamePath;
  size_t                m_serializeSize;
  IGameAudioCallback*   m_audio;               // The audio callback passed to OpenFile()
  IGameVideoCallback*   m_video;               // The video callback passed to OpenFile()
  CGameClientTiming     m_timing;              // Class to scale playback to avoid resampling audio
  PERIPHERALS::EventRateHandle m_inputRateHandle; // Handle while keeping the input sampling rate at the frame rate
  std::unique_ptr<IGameClientPlayback> m_playback; // Interface to control playback
  GAME_REGION           m_region;              // Region of the loaded game

  // Input
  std::vector<CGameClientInput*> m_ports;

  CCriticalSection m_critSection;
};

} // namespace GAME
