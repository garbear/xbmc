/*
 *      Copyright (C) 2012-2014 Team XBMC
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
#include "addons/Addon.h"
#include "addons/AddonDll.h"
#include "addons/DllGameClient.h"
#include "games/GameTypes.h"
#include "games/SerialState.h"
#include "input/joysticks/IJoystickInputHandler.h"
#include "threads/CriticalSection.h"

#include <map>
#include <set>
#include <string>

#define LIBRETRO_WRAPPER_LIBRARY   "game.libretro"

class CFileItem;
class IPlayer;

namespace GAME
{

class CGameClient;

class CDeviceInput : public IJoystickInputHandler
{
public:
  CDeviceInput(CGameClient* addon, int port, const std::string& strDeviceId);

  // Implementation of IJoystickInputHandler
  virtual bool OnButtonPress(JoystickFeatureID id, bool bPressed);
  virtual bool OnButtonMotion(JoystickFeatureID id, float magnitude);
  virtual bool OnAnalogStickMotion(JoystickFeatureID id, float x, float y);
  virtual bool OnAccelerometerMotion(JoystickFeatureID id, float x, float y, float z);

private:
  CGameClient* const m_addon;
  const int          m_port;
  std::string        m_strDeviceId; // TODO
};

class CGameClient : public ADDON::CAddonDll<DllGameClient, GameClient, game_client_properties>
{
public:
  CGameClient(const ADDON::AddonProps& props);
  CGameClient(const cp_extension_t* props);
  virtual ~CGameClient(void);

  // Implementation of CAddon
  virtual ADDON::AddonPtr   GetRunningInstance() const;
  virtual void              OnEnabled();
  virtual void              OnDisabled();
  virtual const std::string LibPath() const; // TODO: Don't make this function virtual, find another way

  // Query properties of the game client
  const std::set<std::string>& GetExtensions() const    { return m_extensions; }
  bool                         SupportsVFS() const      { return m_bSupportsVFS; }
  bool                         SupportsNoGame() const   { return m_bSupportsNoGame; }
  //const GamePlatforms&         GetPlatforms() const     { return m_platforms; }

  // Optimistically returns true if the game client provided no extensions
  bool                         IsExtensionValid(const std::string& strExtension) const;

  // Path to the game client library (ODO: Remove me)
  const std::string&           GameClientPath() const   { return m_strGameClientPath; }

  // Query properties of the running game
  bool               IsPlaying() const     { return m_bIsPlaying; }
  const std::string& GetFilePath() const   { return m_filePath; }
  IPlayer*           GetPlayer() const     { return m_player; }
  int                GetRegion() const     { return m_region; }
  double             GetFrameRate() const  { return m_frameRate * m_frameRateCorrection; }
  double             GetSampleRate() const { return m_sampleRate; }

  // Modify the value returned by GetFrameRate(), used to sync gameplay to audio
  // TODO: Remove me
  void SetFrameRateCorrection(double correctionFactor);

  /**
   * Perform the gamut of checks on the file: "gameclient" property, platform,
   * extension, and a positive match on at least one of the CGameFileLoader
   * strategies.
   */
  bool CanOpen(const CFileItem& file) const;

  // Game API functions
  bool OpenFile(const CFileItem& file, IPlayer* player);
  void Reset();
  void CloseFile();
  bool RunFrame();
  unsigned int RewindFrames(unsigned int frames); // Returns number of frames rewound
  size_t GetAvailableFrames() const { return m_bRewindEnabled ? m_serialState.GetFramesAvailable() : 0; }
  size_t GetMaxFrames() const { return m_bRewindEnabled ? m_serialState.GetMaxFrames() : 0; }

  bool OpenPort(int port, const std::string& strDeviceId);
  void ClosePort(int port);
  void UpdatePort(int port, bool bConnected);

  bool OnButtonPress(int port, JoystickFeatureID id, bool bPressed);
  bool OnButtonMotion(int port, JoystickFeatureID id, float magnitude);
  bool OnAnalogStickMotion(int port, JoystickFeatureID id, float x, float y);
  bool OnAccelerometerMotion(int port, JoystickFeatureID id, float x, float y, float z);

private:
  // Called by the constructors
  void InitializeProperties(void);

  // Private Game API functions
  bool LoadGameInfo();
  bool InitSerialization();

  // Helper functions
  static std::vector<std::string> ParseExtensions(const std::string& strExtensionList);
  //void SetPlatforms(const std::string& strPlatformList);
  bool LogError(GAME_ERROR error, const char* strMethod) const;
  void LogException(const char* strFunctionName) const;
  void LogAddonProperties(void) const; // Unused currently
  static const char* ToString(GAME_ERROR error);

  ADDON::AddonVersion   m_apiVersion;
  CGameClientProperties m_libraryProps;        // Properties to pass to the DLL
  const std::string     m_strGameClientPath;   // Path to the game client library

  // Game API xml parameters
  std::set<std::string> m_extensions;
  bool                  m_bSupportsVFS;
  bool                  m_bSupportsNoGame;
  //GamePlatforms         m_platforms;

  // Properties of the current playing file
  bool                  m_bIsPlaying;          // This is true between OpenFile() and CloseFile()
  std::string           m_filePath;            // The current playing file
  IPlayer*              m_player;              // The player core that called OpenFile()
  GAME_REGION           m_region;              // Region of the loaded game
  double                m_frameRate;           // Video framerate
  double                m_frameRateCorrection; // Framerate correction factor (to sync to audio)
  double                m_sampleRate;          // Audio frequency

  // Save/rewind functionality
  unsigned int          m_serializeSize;
  bool                  m_bRewindEnabled;
  CSerialState          m_serialState;

  // Input
  std::map<int, CDeviceInput*>  m_devices; // port -> controller

  CCriticalSection      m_critSection;
};

} // namespace GAME
