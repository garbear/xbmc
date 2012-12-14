/*
 *      Copyright (C) 2012 Garrett Brown
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GameClient.h"
#include "addons/AddonManager.h"
#include "filesystem/File.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <limits>

using namespace ADDON;

CGameClient::DataReceiver::SetPixelFormat_t CGameClient::SetPixelFormat = NULL;

CGameClient::CGameClient(const AddonProps &props) : CAddon(props)
{
  Initialize();
}

CGameClient::CGameClient(const cp_extension_t *ext) : CAddon(ext)
{
  Initialize();
  if (ext)
  {
    // Extensions list may be comma separated
    CStdString systems = CAddonMgr::Get().GetExtValue(ext->configuration, "system@name");
    StringUtils::SplitString(systems, ",", m_systems);
  }

  // If library attribute isn't present, look for a system-dependent one
  if (ext && m_strLibName.IsEmpty())
  {
#if defined(TARGET_ANDROID)
  m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_android");
#elif defined(_LINUX) && !defined(TARGET_DARWIN)
    m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_linux");
#elif defined(_WIN32) && defined(HAS_SDL_OPENGL)
    m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_wingl");
#elif defined(_WIN32) && defined(HAS_DX)
    m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_windx");
#elif defined(TARGET_DARWIN)
    m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_osx");
#endif
  }
}

void CGameClient::Initialize()
{
  m_bIsInited = false;
  m_bIsPlaying = false;
  m_bAllowVFS = false;
  m_bRequireZip = false;
  m_frameRate = 0.0;
  m_sampleRate = 0.0;
  m_region = -1; // invalid
}

bool CGameClient::Init()
{
  DeInit();
  
  m_dll.SetFile(LibPath());
  m_dll.EnableDelayedUnload(false);
  if (!m_dll.Load())
  {
    CLog::Log(LOGERROR, "GameClient: Error loading DLL %s", LibPath().c_str());
    return false;
  }

  struct retro_system_info info;
  m_dll.retro_get_system_info(&info);
  m_clientName      = info.library_name ? info.library_name : "Unknown";
  m_clientVersion   = info.library_version ? info.library_version : "v0.0";
  m_validExtensions = info.valid_extensions ? info.valid_extensions : "";
  m_bAllowVFS       = !info.need_fullpath;
  m_bRequireZip     = info.block_extract;
  CLog::Log(LOGINFO, "GameClient: Loaded %s core at version %s", m_clientName.c_str(), m_clientVersion.c_str());

  // Verify API versions
  if (m_dll.retro_api_version() != RETRO_API_VERSION)
  {
    CLog::Log(LOGERROR, "GameClient: API version error: XBMC is at version %d, %s is at version %d", RETRO_API_VERSION, m_clientName.c_str(), m_dll.retro_api_version());
    DeInit();
    return false;
  }

  CLog::Log(LOGINFO, "GameClient: ------------------------------------");
  CLog::Log(LOGINFO, "GameClient: Loaded DLL for %s", ID().c_str());
  CLog::Log(LOGINFO, "GameClient: Client: %s at version %s", m_clientName.c_str(), m_clientVersion.c_str());
  CLog::Log(LOGINFO, "GameClient: Valid extensions: %s", m_validExtensions.c_str());
  CLog::Log(LOGINFO, "GameClient: Allow VFS: %s, require zip: %s", m_bAllowVFS ? "yes" : "no", m_bRequireZip ? "yes" : "no");
  CLog::Log(LOGINFO, "GameClient: ------------------------------------");

  return true;
}

void CGameClient::DeInit()
{
  if (m_dll.IsLoaded())
  {
    m_dll.retro_unload_game();
    m_bIsPlaying = false;
    m_dll.retro_deinit();
    m_bIsInited = false;
    try
    {
      m_dll.Unload();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "GameClient: Error unloading DLL: %s", e.what());
    }
  }
}

bool CGameClient::CanOpen(const CStdString &filePath, bool checkExtension /* = true */) const
{
  // Precondition: Init() must have been called
  if (!m_dll.IsLoaded())
    return false;

  if (!m_bAllowVFS)
  {
    // Test if path is a local file
    CURL url(filePath);
    if (!url.GetProtocol().IsEmpty())
      return false;
  }

  // If extensions were specified, make sure we match
  if (checkExtension && !m_validExtensions.empty())
  {
    CStdString strExtension;
    URIUtils::GetExtension(filePath, strExtension);
    strExtension.TrimLeft(".");

    CStdStringArray parts;
    StringUtils::SplitString(m_validExtensions, "|", parts);

    for (CStdStringArray::iterator it = parts.begin(); it != parts.end(); it++)
    {
      if (strExtension.Equals(*it))
        return true; // Extension found
    }
    return false; // Extension not found
  }
  return true; // DLL wasn't kind enough to provide extensions, assume the best
}

bool CGameClient::OpenFile(const CFileItem& file, const DataReceiver &callbacks)
{
  // Can't open a file without first initializing the DLL...
  if (!m_dll.IsLoaded())
    Init();

  if (!CanOpen(file.GetPath()))
      return false;
  
  // Install the pixel format hook. This is called by EnvironmentCallback().
  SetPixelFormat = callbacks.SetPixelFormat;

  // Because we call m_dll.retro_init() here instead of in Init(), keep track
  // of this. Note that if we return false later, m_bIsInited will still be
  // true because the DLL is still loaded and inited
  if (!m_bIsInited)
  {
    // Set up our callback to retrieve environment variables
    // retro_set_environment() must be called before retro_init()
    m_dll.retro_set_environment(EnvironmentCallback);
    m_dll.retro_init();
    m_bIsInited = true;
  }

  const char * path = NULL;
  void *       data = NULL;
  uint64_t     length = 0;

  // Use the vfs if it's allowed
  if (m_bAllowVFS)
  {
    // If m_bRequireZip is true, the client prefers (or possibly requires) zip files
    // because they may contain important files. In that case, avoid the vfs
    CStdString strExtension;
    URIUtils::GetExtension(file.GetPath(), strExtension);
    if (!(m_bRequireZip && strExtension.CompareNoCase(".zip") == 0))
    {
      XFILE::CFile vfsFile;
      CStdString strFilePath = file.GetPath();
      if (!vfsFile.Open(strFilePath))
        return false;
      length = vfsFile.GetLength();
      // Check for file size overflow (libretro accepts files <= size_t max)
      if (length >= std::numeric_limits<size_t>::max())
        return false;

      data = new char[(size_t)length];
      if (data)
      {
        if (length != vfsFile.Read(data, length))
        {
          delete[] (char*)data;
          return false;
        }
      }
    }
  }

  if (!data)
  {
    path = file.GetPath().c_str();
    length = 0;
  }

  // This is the structure we fill with info about the file we are loading
  struct retro_game_info info;
  info.path = path; // String or NULL if using info.data
  info.data = data; // Pointer to full file loaded into memory
  info.size = (size_t)length; // Size of info.data
  info.meta = NULL; // Client-specific meta-data; XML memory map

  bool ret, useMultipleRoms = false;
  if (!useMultipleRoms)
  {
    ret = m_dll.retro_load_game(&info);
  }
  else
  {
    // Special game types used when multiple roms are required
    unsigned int romType = RETRO_GAME_TYPE_BSX;
    //unsigned int romType = RETRO_GAME_TYPE_BSX_SLOTTED;
    //unsigned int romType = RETRO_GAME_TYPE_SUFAMI_TURBO;
    //unsigned int romType = RETRO_GAME_TYPE_SUPER_GAME_BOY;
    ret = m_dll.retro_load_game_special(romType, &info, 1);
  }

  delete[] (char*)data;

  // If ret is 0, we failed to load the game
  if (!ret)
  {
    if (path)
      CLog::Log(LOGERROR, "GameClient: Failed to load game by path: %s", path);
    else
      CLog::Log(LOGERROR, "GameClient: Failed to load game through VFS (filesize: %d KB)", (int)length / 1024);
    return false;
  }

  m_bIsPlaying = true;

  // Get information about system audio/video timings and geometry
  // Can be called only after retro_load_game()
  struct retro_system_av_info av_info;
  m_dll.retro_get_system_av_info(&av_info);

  unsigned int baseWidth  = av_info.geometry.base_width; // 256
  unsigned int baseHeight = av_info.geometry.base_height; // 224
  unsigned int maxWidth   = av_info.geometry.max_width; // 512
  unsigned int maxHeight  = av_info.geometry.max_height; // 448
  float aspectRatio       = av_info.geometry.aspect_ratio; // 0.0
  double fps              = av_info.timing.fps; // 60.098811862348406
  double sampleRate       = av_info.timing.sample_rate; // 32040.5

  CLog::Log(LOGDEBUG, "GameClient: ---------------------------------------");
  CLog::Log(LOGDEBUG, "GameClient: Opened file %s", file.GetPath().c_str());
  CLog::Log(LOGDEBUG, "GameClient: Base Width: %u", baseWidth);
  CLog::Log(LOGDEBUG, "GameClient: Base Height: %u", baseHeight);
  CLog::Log(LOGDEBUG, "GameClient: Max Width: %u", maxWidth);
  CLog::Log(LOGDEBUG, "GameClient: Max Height: %u", maxHeight);
  CLog::Log(LOGDEBUG, "GameClient: Aspect Ratio: %f", aspectRatio);
  CLog::Log(LOGDEBUG, "GameClient: FPS: %f", fps);
  CLog::Log(LOGDEBUG, "GameClient: Sample Rate: %f", sampleRate);
  CLog::Log(LOGDEBUG, "GameClient: ---------------------------------------");
  
  m_frameRate = fps;
  m_sampleRate = sampleRate;

  // Query the game region
  switch (m_dll.retro_get_region())
  {
  case RETRO_REGION_NTSC:
    m_region = RETRO_REGION_NTSC;
    break;
  case RETRO_REGION_PAL:
    m_region = RETRO_REGION_PAL;
    break;
  default:
    break;
  }

  // Load save and auto state

  // Install callbacks
  m_dll.retro_set_video_refresh(callbacks.VideoFrame);
  m_dll.retro_set_audio_sample(callbacks.AudioSample);
  m_dll.retro_set_audio_sample_batch(callbacks.AudioSampleBatch);
  m_dll.retro_set_input_state(callbacks.GetInputState);
  m_dll.retro_set_input_poll(NoopPoop);

  SetDevice(0, RETRO_DEVICE_JOYPAD);

  return true;
}

void CGameClient::CloseFile()
{
  if (m_dll.IsLoaded())
  {
    m_dll.retro_unload_game();
    m_bIsPlaying = false;
  }
}

void CGameClient::SetDevice(unsigned int port, unsigned int device)
{
  if (m_bIsPlaying)
  {
    // Validate port (TODO: Check if port is less that players that individual game client supports)
    if (port < GAMECLIENT_MAX_PLAYERS)
    {
      // Validate device
      if (device <= RETRO_DEVICE_ANALOG ||
          device == RETRO_DEVICE_JOYPAD_MULTITAP ||
          device == RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE ||
          device == RETRO_DEVICE_LIGHTGUN_JUSTIFIER ||
          device == RETRO_DEVICE_LIGHTGUN_JUSTIFIERS)
      {
        m_dll.retro_set_controller_port_device(port, device);
      }
    }
  }
}

void CGameClient::RunFrame()
{
  if (m_bIsPlaying)
    m_dll.retro_run();
}

void CGameClient::Reset()
{
  if (m_bIsPlaying)
  {
    // TODO: Reset all controller ports to their same value. bSNES since v073r01
    // resets controllers to JOYPAD after a reset, so guard against this.
    m_dll.retro_reset();
  }
}

bool CGameClient::EnvironmentCallback(unsigned int cmd, void *data)
{
  // Note: RETRO_ENVIRONMENT_SHUTDOWN doesn't use data and the other uses data as a return path
  if (!data && cmd != RETRO_ENVIRONMENT_SHUTDOWN && cmd != RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY)
  {
    CLog::Log(LOGERROR, "GameClient environment query ID=%d: no data! naughty core?", cmd);
    return false;
  }

  switch (cmd)
  {
  case RETRO_ENVIRONMENT_GET_OVERSCAN:
    {
      // Whether or not the game client should use overscan (true) or crop away overscan (false)
      *reinterpret_cast<bool*>(data) = false;
      CLog::Log(LOGINFO, "GameClient environment query ID=%d: %s", RETRO_ENVIRONMENT_GET_OVERSCAN,
          *reinterpret_cast<bool*>(data) ? "use overscan" : "crop away overscan");
      break;
    }
  case RETRO_ENVIRONMENT_GET_CAN_DUPE:
    {
      // Boolean value whether or not we support frame duping, passing NULL to video frame callback
      *(bool*)data = true;
      CLog::Log(LOGINFO, "GameClient environment query ID=%d: frame duping is %s",
          RETRO_ENVIRONMENT_GET_CAN_DUPE, *reinterpret_cast<bool*>(data) ? "enabled" : "disabled");
      break;
    }
  case RETRO_ENVIRONMENT_GET_VARIABLE:
    {
      // Interface to acquire user-defined information from environment that cannot feasibly be
      // supported in a multi-system way. Mostly used for obscure, specific features that the
      // user can tap into when necessary.
      struct retro_variable *var = reinterpret_cast<struct retro_variable*>(data);
      if (var->key)
      {
        // For example...
        if (var->key && strncmp("too_sexy_for", var->key, 12))
        {
          var->value = "shirt";
          CLog::Log(LOGINFO, "GameClient environment query ID=%d: variable %s set to %s",
              RETRO_ENVIRONMENT_GET_VARIABLE, var->key, var->value);
        }
        else
        {
          CLog::Log(LOGERROR, "GameClient environment query ID=%d: undefined variable %s",
              RETRO_ENVIRONMENT_GET_VARIABLE, var->key);
          var->value = NULL;
        }
      }
      else
      {
        CLog::Log(LOGERROR, "GameClient environment query ID=%d: no variable given",
            RETRO_ENVIRONMENT_GET_VARIABLE);
        var->value = NULL;
      }
      break;
    }
  case RETRO_ENVIRONMENT_SET_VARIABLES:
    {
      // Allows an implementation to signal the environment which variables it might want to check
      // for later using GET_VARIABLE. 'data' points to an array of retro_variable structs terminated
      // by a { NULL, NULL } element. retro_variable::value should contain a human readable description
      // of the key.
      const struct retro_variable *vars = reinterpret_cast<const struct retro_variable*>(data);
      if (!vars->key)
      {
        CLog::Log(LOGERROR, "GameClient environment query ID=%d: no variables given",
            RETRO_ENVIRONMENT_SET_VARIABLES);
      }
      else
      {
        while (vars->key)
        {
          if (vars->value)
            CLog::Log(LOGINFO, "GameClient environment query ID=%d: notified of var %s (%s)",
              RETRO_ENVIRONMENT_SET_VARIABLES, vars->key, vars->value);
          else
            CLog::Log(LOGWARNING, "GameClient environment query ID=%d: var %s has no description",
              RETRO_ENVIRONMENT_SET_VARIABLES, vars->key);
          vars++;
        }
      }
      break;
    }
  case RETRO_ENVIRONMENT_SET_MESSAGE:
    {
      // Sets a message to be displayed. Generally not for trivial messages.
      const struct retro_message *msg = reinterpret_cast<const struct retro_message*>(data);
      CLog::Log(LOGINFO, "GameClient environment query ID=%d: display msg \"%s\" for %d frames",
          RETRO_ENVIRONMENT_SET_MESSAGE, msg->msg, msg->frames);
      break;
    }
  case RETRO_ENVIRONMENT_SET_ROTATION:
    {
      // Sets screen rotation of graphics. Valid values are 0, 1, 2, 3, which rotates screen
      // by 0, 90, 180, 270 degrees counter-clockwise respectively.
      unsigned int rotation = *reinterpret_cast<const unsigned int*>(data);
      if (0 <= rotation && rotation <= 3)
        CLog::Log(LOGINFO, "GameClient environment query ID=%d: set screen rotation to %d degrees",
            RETRO_ENVIRONMENT_SET_ROTATION, rotation * 90);
      else
        CLog::Log(LOGERROR, "GameClient environment query ID=%d: invalid rotation %d",
            RETRO_ENVIRONMENT_SET_ROTATION, rotation);
      break;
    }
  case RETRO_ENVIRONMENT_SHUTDOWN:
    // Game has been shut down. Should only be used if game has a specific way to shutdown
    // the game from a menu item or similar.
    CLog::Log(LOGINFO, "GameClient environment query ID=%d: game signaled shutdown event", RETRO_ENVIRONMENT_SHUTDOWN);
    break;
  case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
    {
      // Generally how computationally intense this core is, to gauge how capable XBMC's host system
      // will be for running the core. It can also be called on a game-specific basis. The levels
      // are "floating", but roughly defined as:
      // 0: Low-powered embedded devices such as Raspberry Pi
      // 1: Phones, tablets, 6th generation consoles, such as Wii/Xbox 1, etc
      // 2: 7th generation consoles, such as PS3/360, with sub-par CPUs.
      // 3: Modern desktop/laptops with reasonably powerful CPUs.
      // 4: High-end desktops with very powerful CPUs.
      unsigned int performanceLevel = *reinterpret_cast<const unsigned int*>(data);
      if (0 <= performanceLevel && performanceLevel <= 3)
        CLog::Log(LOGINFO, "GameClient environment query ID=%d: performance hint: %d",
            RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, performanceLevel);
      else if (performanceLevel == 4)
        CLog::Log(LOGINFO, "GameClient environment query ID=%d: performance hint: I hope you have a badass computer...",
            RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL);
      else
        CLog::Log(LOGERROR, "GameClient environment query ID=%d: invalid performance hint: %d",
            RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, performanceLevel);
      break;
    }
  case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    {
      // Returns a directory for storing system specific ROMs such as BIOSes, configuration data,
      // etc. The returned value can be NULL, in which case it's up to the implementation to find
      // a suitable directory.
      *reinterpret_cast<const char**>(data) = NULL;
      CLog::Log(LOGINFO, "GameClient environment query ID=%d: no system directory given to core",
          RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY);
      break;
    }
  case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
    {
      // Get the internal pixel format used by the core. The default pixel format is
      // RETRO_PIXEL_FORMAT_0RGB1555 (see libretro.h). Returning false lets the core
      // know that XBMC does not support the pixel format.
      retro_pixel_format pix_fmt = *reinterpret_cast<const retro_pixel_format*>(data);
      // Validate the format
      switch (pix_fmt)
      {
      case RETRO_PIXEL_FORMAT_RGB565:
      case RETRO_PIXEL_FORMAT_0RGB1555: // 5 bit color, high byte must be zero
      case RETRO_PIXEL_FORMAT_XRGB8888: // 8 bit color, high byte is ignored
        SetPixelFormat(pix_fmt);
        break;
      default:
        return false;
      }
      break;
    }
  default:
    CLog::Log(LOGERROR, "GameClient environment query ID=%d: invalid query: %d",
        RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, cmd);
    return false;
  }
  return true;
}
