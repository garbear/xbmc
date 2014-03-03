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
#include "addons/Addon.h"
#include "addons/AddonDll.h"
#include "addons/DllGameClient.h"
#include "games/controllers/ControllerTypes.h"
#include "games/GameTypes.h"
#include "games/SerialState.h"
#include "input/keyboard/IKeyboardHandler.h"
#include "input/Key.h"
#include "peripherals/EventScanRate.h"
#include "threads/CriticalSection.h"

#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"

#include <map>
//#include <queue>
#include <set>
#include <string>
#include <vector>

#define GAME_STREAM_VIDEO_ID  1
#define GAME_STREAM_AUDIO_ID  2

class CAEChannelInfo;
class CFileItem;
struct DemuxPacket;

namespace GAME
{

class CGameClientInput;

class IGameDemuxCallback
{
public:
  virtual ~IGameDemuxCallback() { }

  virtual void OpenVideoStream(AVCodecID codec, AVPixelFormat pixfmt, unsigned int width, unsigned int height) = 0;
  virtual void CloseVideoStream() = 0;
  virtual void OpenAudioStream(AVCodecID codec, unsigned int samplerate, const CAEChannelInfo& channelLayout) = 0;
  virtual void CloseAudioStream() = 0;
};

class CGameClient : public ADDON::CAddonDll<DllGameClient, GameClient, game_client_properties>,
                    public KEYBOARD::IKeyboardHandler
{
public:
  enum class STREAM_TYPE
  {
    NONE,
    VIDEO,
    AUDIO,
  };

  static std::unique_ptr<CGameClient> FromExtension(ADDON::AddonProps props, const cp_extension_t* ext);

  CGameClient(ADDON::AddonProps props, const std::vector<std::string>& extensions, bool bSupportsVFS, bool bSupportsStandalone, bool bSupportsKeyboard);

  virtual ~CGameClient(void);

  bool Initialize(void);

  // Implementation of IAddon via CAddonDll
  virtual bool              IsType(ADDON::TYPE type) const override;
  virtual std::string       LibPath() const override;
  virtual void              OnDisabled() override;
  virtual void              OnEnabled() override;
  virtual ADDON::AddonPtr   GetRunningInstance() const override;

  // Query properties of the game client
  const std::set<std::string>& GetExtensions() const    { return m_extensions; }
  bool                         SupportsVFS() const      { return m_bSupportsVFS; }
  //const GamePlatforms&         GetPlatforms() const     { return m_platforms; }
  bool                         IsExtensionValid(const std::string& strExtension) const;
  bool                         IsStandalone() const { return m_bSupportsStandalone; }

  // Query properties of the running game
  bool               IsPlaying() const     { return m_bIsPlaying; }
  const std::string& GetFilePath() const   { return m_filePath; }
  GAME_REGION        GetRegion() const     { return m_region; }

  bool CanOpen(const CFileItem& file) const;

  // Game API functions
  bool OpenFile(const CFileItem& file, IGameDemuxCallback* demuxer);
  void Reset();
  void CloseFile();
  bool OpenVideoStream(AVCodecID codec, AVPixelFormat pixelFormat, unsigned int width, unsigned int height);
  void AddVideoData(const uint8_t* data, unsigned int size);
  void CloseVideoStream();
  bool OpenAudioStream(AVCodecID codec, unsigned int samplerate, const CAEChannelInfo& channelLayout);
  void AddAudioData(const uint8_t* data, unsigned int size);
  void CloseAudioStream();
  DemuxPacket* ReadDemux();
  void FlushDemux();

  unsigned int RewindFrames(unsigned int frames); // Returns number of frames rewound
  size_t GetAvailableFrames() const { return m_bRewindEnabled ? m_serialState.GetFramesAvailable() : 0; }
  size_t GetMaxFrames() const { return m_bRewindEnabled ? m_serialState.GetMaxFrames() : 0; }

  bool OpenPort(unsigned int port);
  void ClosePort(unsigned int port);
  void UpdatePort(unsigned int port, const ControllerPtr& controller);
  bool HasFeature(const std::string& controller, const std::string& feature);

  bool OnButtonPress(int port, const std::string& feature, bool bPressed);
  bool OnButtonMotion(int port, const std::string& feature, float magnitude);
  bool OnAnalogStickMotion(int port, const std::string& feature, float x, float y);
  bool OnAccelerometerMotion(int port, const std::string& feature, float x, float y, float z);

  // implementation of IKeyboardHandler
  virtual bool OnKeyPress(const CKey& key) override;
  virtual void OnKeyRelease(const CKey& key) override;

private:
  // Called by the constructors
  void InitializeProperties(void);

  // Private Game API functions
  bool LoadGameInfo();
  bool InitSerialization();

  void ClearPorts(void);
  void OpenKeyboard(void);
  void CloseKeyboard(void);
  ControllerVector GetControllers(void) const;

  // Helper functions
  static std::vector<std::string> ParseExtensions(const std::string& strExtensionList);
  //void SetPlatforms(const std::string& strPlatformList);
  static GAME_KEY_MOD GetModifiers(CKey::Modifier modifier);
  bool LogError(GAME_ERROR error, const char* strMethod) const;
  void LogException(const char* strFunctionName) const;
  void LogAddonProperties(void) const; // Unused currently
  static const char* ToString(GAME_ERROR error);

  ADDON::AddonVersion   m_apiVersion;
  CGameClientProperties m_libraryProps;        // Properties to pass to the DLL

  // Game API xml parameters
  std::set<std::string> m_extensions;
  bool                  m_bSupportsVFS;
  bool                  m_bSupportsStandalone;
  bool                  m_bSupportsKeyboard;
  //GamePlatforms         m_platforms;

  // Properties of the current playing file
  bool                  m_bIsPlaying;          // This is true between OpenFile() and CloseFile()
  std::string           m_filePath;            // The current playing file
  IGameDemuxCallback*   m_demuxer;             // The demuxer passed to OpenFile()
  PERIPHERALS::EventRateHandle m_inputRateHandle; // Handle while keeping the input sampling rate at the frame rate
  GAME_REGION           m_region;              // Region of the loaded game
  double                m_frameRate;           // Video framerate
  double                m_sampleRate;          // Audio frequency

  enum STREAM_ID
  {
    STREAM_VIDEO,
    STREAM_AUDIO,
  };

  struct StreamPacket
  {
    STREAM_ID  streamId;
    uint8_t*   pData;
    size_t     size;
  };

  // Video/audio streams
  bool                     m_bHasVideo;           // True if a video stream is open
  bool                     m_bHasRawVideo;        // True if video stream is raw pixels, false for encoded data
  bool                     m_bHasAudio;           // True if an audio stream is open
  std::vector<DemuxPacket*> m_buffer;
  CCriticalSection         m_demuxMutex;
  CEvent                   m_demuxEvent;

  // Save/rewind functionality
  bool                  m_bSerializationInited;
  unsigned int          m_serializeSize;
  bool                  m_bRewindEnabled;
  CSerialState          m_serialState;

  // Input
  std::vector<CGameClientInput*> m_controllers;

  CCriticalSection      m_critSection;
};

} // namespace GAME
