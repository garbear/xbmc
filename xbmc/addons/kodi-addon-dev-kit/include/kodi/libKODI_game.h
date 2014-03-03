/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "libXBMC_addon.h"
#include "kodi_game_callbacks.h"

#include <string>
#include <stdio.h>

#if defined(ANDROID)
  #include <sys/stat.h>
#endif

#ifdef _WIN32
  #define GAME_HELPER_DLL "\\library.kodi.game\\libKODI_game" ADDON_HELPER_EXT
#else
  #define GAME_HELPER_DLL_NAME "libKODI_game-" ADDON_HELPER_ARCH ADDON_HELPER_EXT
  #define GAME_HELPER_DLL "/library.kodi.game/" GAME_HELPER_DLL_NAME
#endif

#define GAME_REGISTER_SYMBOL(dll, functionPtr) \
  CHelper_libKODI_game::RegisterSymbol(dll, functionPtr, #functionPtr)

class CHelper_libKODI_game
{
public:
  CHelper_libKODI_game(void) :
    GAME_register_me(nullptr),
    GAME_unregister_me(nullptr),
    GAME_close_game(nullptr),
    GAME_open_video_stream(nullptr),
    GAME_add_video_data(nullptr),
    GAME_close_video_stream(nullptr),
    GAME_open_audio_stream(nullptr),
    GAME_add_audio_data(nullptr),
    GAME_close_audio_stream(nullptr),
    GAME_hw_set_info(nullptr),
    GAME_hw_get_current_framebuffer(nullptr),
    GAME_hw_get_proc_address(nullptr),
    GAME_open_port(nullptr),
    GAME_close_port(nullptr),
    GAME_rumble_set_state(nullptr),
    m_handle(nullptr),
    m_callbacks(nullptr),
    m_libKODI_game(nullptr)
  {
  }

  ~CHelper_libKODI_game(void)
  {
    if (m_libKODI_game)
    {
      GAME_unregister_me(m_handle, m_callbacks);
      dlclose(m_libKODI_game);
    }
  }

  template <typename T>
  static bool RegisterSymbol(void* dll, T& functionPtr, const char* strFunctionPtr)
  {
    return (functionPtr = (T)dlsym(dll, strFunctionPtr)) != NULL;
  }

  /*!
    * @brief Resolve all callback methods
    * @param handle Pointer to the add-on
    * @return True when all methods were resolved, false otherwise.
    */
  bool RegisterMe(void* handle)
  {
    m_handle = handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_handle)->libBasePath;
    libBasePath += GAME_HELPER_DLL;

#if defined(ANDROID)
      struct stat st;
      if (stat(libBasePath.c_str(),&st) != 0)
      {
        std::string tempbin = getenv("XBMC_ANDROID_LIBS");
        libBasePath = tempbin + "/" + GAME_HELPER_DLL_NAME;
      }
#endif

    m_libKODI_game = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libKODI_game == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    try
    {
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_register_me)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_unregister_me)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_close_game)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_open_video_stream)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_add_video_data)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_close_video_stream)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_open_audio_stream)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_add_audio_data)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_close_audio_stream)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_hw_set_info)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_hw_get_current_framebuffer)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_hw_get_proc_address)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_open_port)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_close_port)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_rumble_set_state)) throw false;
    }
    catch (const bool& bSuccess)
    {
      fprintf(stderr, "ERROR: Unable to assign function %s\n", dlerror());
      return bSuccess;
    }

    m_callbacks = GAME_register_me(m_handle);
    return m_callbacks != NULL;
  }

  void CloseGame(void)
  {
    return GAME_close_game(m_handle, m_callbacks);
  }

  bool OpenVideoStream(GAME_VIDEO_FORMAT format, unsigned int width, unsigned int height)
  {
    return GAME_open_video_stream(m_handle, m_callbacks, format, width, height) == 0;
  }

  void AddVideoData(const uint8_t* data, unsigned int size)
  {
    GAME_add_video_data(m_handle, m_callbacks, data, size);
  }

  void CloseVideoStream()
  {
    GAME_close_video_stream(m_handle, m_callbacks);
  }

  bool OpenAudioStream(GAME_AUDIO_FORMAT format, unsigned int samplerate, GAME_AUDIO_CHANNEL_LAYOUT channels)
  {
    return GAME_open_audio_stream(m_handle, m_callbacks, format, samplerate, channels) == 0;
  }

  void AddAudioData(const uint8_t* data, unsigned int size)
  {
    GAME_add_audio_data(m_handle, m_callbacks, data, size);
  }

  void CloseAudioStream()
  {
    GAME_close_audio_stream(m_handle, m_callbacks);
  }

  void HwSetInfo(const struct game_hw_info* hw_info)
  {
    return GAME_hw_set_info(m_handle, m_callbacks, hw_info);
  }

  uintptr_t HwGetCurrentFramebuffer(void)
  {
    return GAME_hw_get_current_framebuffer(m_handle, m_callbacks);
  }

  game_proc_address_t HwGetProcAddress(const char* sym)
  {
    return GAME_hw_get_proc_address(m_handle, m_callbacks, sym);
  }

  bool OpenPort(unsigned int port)
  {
    return GAME_open_port(m_handle, m_callbacks, port);
  }

  void ClosePort(unsigned int port)
  {
    return GAME_close_port(m_handle, m_callbacks, port);
  }

  void RumbleSetState(unsigned int port, GAME_RUMBLE_EFFECT effect, float strength)
  {
    return GAME_rumble_set_state(m_handle, m_callbacks, port, effect, strength);
  }

protected:
  CB_GameLib* (*GAME_register_me)(void* handle);
  void (*GAME_unregister_me)(void* handle, CB_GameLib* cb);
  void (*GAME_close_game)(void* handle, CB_GameLib* cb);
  int (*GAME_open_video_stream)(void* handle, CB_GameLib* cb, GAME_VIDEO_FORMAT, unsigned int, unsigned int);
  int (*GAME_add_video_data)(void* handle, CB_GameLib* cb, const uint8_t*, unsigned int);
  int (*GAME_close_video_stream)(void* handle, CB_GameLib* cb);
  int (*GAME_open_audio_stream)(void* handle, CB_GameLib* cb, GAME_AUDIO_FORMAT, unsigned int, GAME_AUDIO_CHANNEL_LAYOUT);
  int (*GAME_add_audio_data)(void* handle, CB_GameLib* cb, const uint8_t*, unsigned int);
  int (*GAME_close_audio_stream)(void* handle, CB_GameLib* cb);
  void (*GAME_hw_set_info)(void* handle, CB_GameLib* cb, const struct game_hw_info*);
  uintptr_t (*GAME_hw_get_current_framebuffer)(void* handle, CB_GameLib* cb);
  game_proc_address_t (*GAME_hw_get_proc_address)(void* handle, CB_GameLib* cb, const char*);
  bool (*GAME_open_port)(void* handle, CB_GameLib* cb, unsigned int);
  void (*GAME_close_port)(void* handle, CB_GameLib* cb, unsigned int);
  void (*GAME_rumble_set_state)(void* handle, CB_GameLib* cb, unsigned int, GAME_RUMBLE_EFFECT, float);

private:
  void*        m_handle;
  CB_GameLib*  m_callbacks;
  void*        m_libKODI_game;

  struct cb_array
  {
    const char* libBasePath;
  };
};
