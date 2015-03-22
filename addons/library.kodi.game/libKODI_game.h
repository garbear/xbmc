#pragma once
/*
 *      Copyright (C) 2005-2014 Team XBMC
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
  CHelper_libKODI_game(void)
  {
    m_handle       = NULL;
    m_callbacks    = NULL;
    m_libKODI_game = NULL;
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
    libBasePath  = ((cb_array*)m_handle)->libPath;
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
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_open_port)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_close_port)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_environment_set_rotation)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_environment_get_overscan)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_environment_can_dupe)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_environment_set_system_av_info)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_video_frame)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_audio_frames)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_rumble_set_state)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_perf_get_time_usec)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_perf_get_counter)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_perf_get_cpu_features)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_perf_log)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_perf_register)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_perf_start)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_perf_stop)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_camera_set_info)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_camera_start)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_camera_stop)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_location_start)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_location_stop)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_location_get_position)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_location_set_interval)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_location_initialized)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_location_deinitialized)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_frame_time_set_reference)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_hw_set_info)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_hw_get_current_framebuffer)) throw false;
      if (!GAME_REGISTER_SYMBOL(m_libKODI_game, GAME_hw_get_proc_address)) throw false;
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

  bool OpenPort(unsigned int port, const char* addon_id, game_input_device_caps* device_caps)
  {
    return GAME_open_port(m_handle, m_callbacks, port, addon_id, device_caps);
  }

  void ClosePort(unsigned int port)
  {
    return GAME_close_port(m_handle, m_callbacks, port);
  }

  void EnvironmentSetRotation(enum GAME_ROTATION rotation)
  {
    return GAME_environment_set_rotation(m_handle, m_callbacks, rotation);
  }

  bool EnvironmentGetOverscan(void)
  {
    return GAME_environment_get_overscan(m_handle, m_callbacks);
  }

  bool EnvironmentCanDupe(void)
  {
    return GAME_environment_can_dupe(m_handle, m_callbacks);
  }

  bool EnvironmentSetSystemAvInfo(const struct game_system_av_info* info)
  {
    return GAME_environment_set_system_av_info(m_handle, m_callbacks, info);
  }

  bool VideoFrame(GAME_RENDER_FORMAT format, unsigned int width, unsigned int height, const uint8_t* data)
  {
    return GAME_video_frame(m_handle, m_callbacks, format, width, height, data);
  }

  unsigned int AudioFrames(GAME_AUDIO_FORMAT format, unsigned int frames, const uint8_t* data)
  {
    return GAME_audio_frames(m_handle, m_callbacks, format, frames, data);
  }

  bool RumbleSetState(unsigned port, enum GAME_RUMBLE_EFFECT effect, uint16_t strength)
  {
    return GAME_rumble_set_state(m_handle, m_callbacks, port, effect, strength);
  }

  game_time_t PerfGetTimeUsec(void)
  {
    return GAME_perf_get_time_usec(m_handle, m_callbacks);
  }

  game_perf_tick_t PerfGetCounter(void)
  {
    return GAME_perf_get_counter(m_handle, m_callbacks);
  }

  uint64_t PerfGetCpuFeatures(void)
  {
    return GAME_perf_get_cpu_features(m_handle, m_callbacks);
  }

  void PerfLog(void)
  {
    return GAME_perf_log(m_handle, m_callbacks);
  }

  void PerfRegister(struct game_perf_counter *counter)
  {
    return GAME_perf_register(m_handle, m_callbacks, counter);
  }

  void PerfStart(struct game_perf_counter *counter)
  {
    return GAME_perf_start(m_handle, m_callbacks, counter);
  }

  void PerfStop(struct game_perf_counter *counter)
  {
    return GAME_perf_stop(m_handle, m_callbacks, counter);
  }

  void CameraSetInfo(struct game_camera_info *camera_info)
  {
    return GAME_camera_set_info(m_handle, m_callbacks, camera_info);
  }

  bool CameraStart(void)
  {
    return GAME_camera_start(m_handle, m_callbacks);
  }

  void CameraStop(void)
  {
    return GAME_camera_stop(m_handle, m_callbacks);
  }

  bool LocationStart(void)
  {
    return GAME_location_start(m_handle, m_callbacks);
  }

  void LocationStop(void)
  {
    return GAME_location_stop(m_handle, m_callbacks);
  }

  bool LocationGetPosition(double *lat, double *lon, double *horiz_accuracy, double *vert_accuracy)
  {
    return GAME_location_get_position(m_handle, m_callbacks, lat, lon, horiz_accuracy, vert_accuracy);
  }

  void LocationSetInterval(unsigned interval_ms, unsigned interval_distance)
  {
    return GAME_location_set_interval(m_handle, m_callbacks, interval_ms, interval_distance);
  }

  void LocationInitialized(void)
  {
    return GAME_location_initialized(m_handle, m_callbacks);
  }

  void LocationDeinitialized(void)
  {
    return GAME_location_deinitialized(m_handle, m_callbacks);
  }

  void FrameTimeSetReference(game_usec_t usec)
  {
    return GAME_frame_time_set_reference(m_handle, m_callbacks, usec);
  }

  void HwSetInfo(const struct game_hw_info *hw_info)
  {
    return GAME_hw_set_info(m_handle, m_callbacks, hw_info);
  }

  uintptr_t HwGetCurrentFramebuffer(void)
  {
    return GAME_hw_get_current_framebuffer(m_handle, m_callbacks);
  }

  game_proc_address_t HwGetProcAddress(const char *sym)
  {
    return GAME_hw_get_proc_address(m_handle, m_callbacks, sym);
  }

protected:
    CB_GameLib* (*GAME_register_me)(void* handle);
    void (*GAME_unregister_me)(void* handle, CB_GameLib* cb);
    void (*GAME_close_game)(void* handle, CB_GameLib* cb);
    bool (*GAME_open_port)(void* handle, CB_GameLib* cb, unsigned int port, const char* addon_id, game_input_device_caps* device_caps);
    void (*GAME_close_port)(void* handle, CB_GameLib* cb, unsigned int port);
    void (*GAME_environment_set_rotation)(void* handle, CB_GameLib* cb, enum GAME_ROTATION rotation);
    bool (*GAME_environment_get_overscan)(void* handle, CB_GameLib* cb);
    bool (*GAME_environment_can_dupe)(void* handle, CB_GameLib* cb);
    bool (*GAME_environment_set_system_av_info)(void* handle, CB_GameLib* cb, const struct game_system_av_info* info);
    bool (*GAME_video_frame)(void* handle, CB_GameLib* cb, GAME_RENDER_FORMAT format, unsigned int width, unsigned int height, const uint8_t* data);
    unsigned int (*GAME_audio_frames)(void* handle, CB_GameLib* cb, GAME_AUDIO_FORMAT format, unsigned int frames, const uint8_t* data);
    bool (*GAME_rumble_set_state)(void* handle, CB_GameLib* cb, unsigned port, enum GAME_RUMBLE_EFFECT effect, uint16_t strength);
    game_time_t (*GAME_perf_get_time_usec)(void* handle, CB_GameLib* cb);
    game_perf_tick_t (*GAME_perf_get_counter)(void* handle, CB_GameLib* cb);
    uint64_t (*GAME_perf_get_cpu_features)(void* handle, CB_GameLib* cb);
    void (*GAME_perf_log)(void* handle, CB_GameLib* cb);
    void (*GAME_perf_register)(void* handle, CB_GameLib* cb, struct game_perf_counter *counter);
    void (*GAME_perf_start)(void* handle, CB_GameLib* cb, struct game_perf_counter *counter);
    void (*GAME_perf_stop)(void* handle, CB_GameLib* cb, struct game_perf_counter *counter);
    void (*GAME_camera_set_info)(void* handle, CB_GameLib* cb, struct game_camera_info *camera_info);
    bool (*GAME_camera_start)(void* handle, CB_GameLib* cb);
    void (*GAME_camera_stop)(void* handle, CB_GameLib* cb);
    bool (*GAME_location_start)(void* handle, CB_GameLib* cb);
    void (*GAME_location_stop)(void* handle, CB_GameLib* cb);
    bool (*GAME_location_get_position)(void* handle, CB_GameLib* cb, double *lat, double *lon, double *horiz_accuracy, double *vert_accuracy);
    void (*GAME_location_set_interval)(void* handle, CB_GameLib* cb, unsigned interval_ms, unsigned interval_distance);
    void (*GAME_location_initialized)(void* handle, CB_GameLib* cb);
    void (*GAME_location_deinitialized)(void* handle, CB_GameLib* cb);
    void (*GAME_frame_time_set_reference)(void* handle, CB_GameLib* cb, game_usec_t usec);
    void (*GAME_hw_set_info)(void* handle, CB_GameLib* cb, const struct game_hw_info *hw_info);
    uintptr_t (*GAME_hw_get_current_framebuffer)(void* handle, CB_GameLib* cb);
    game_proc_address_t (*GAME_hw_get_proc_address)(void* handle, CB_GameLib* cb, const char *sym);

private:
  void*        m_handle;
  CB_GameLib*  m_callbacks;
  void*        m_libKODI_game;

  struct cb_array
  {
    const char* libPath;
  };
};
