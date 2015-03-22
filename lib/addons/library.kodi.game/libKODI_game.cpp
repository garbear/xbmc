/*
 *      Copyright (C) 2014 Team XBMC
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

#include "addons/AddonCallbacks.h"
#include "addons/include/kodi_game_types.h"

#include <stdio.h>

#ifdef _WIN32
  #include <windows.h>
  #define DLLEXPORT __declspec(dllexport)
#else
  #define DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT CB_GameLib* GAME_register_me(AddonCB* frontend)
{
  CB_GameLib* cb = NULL;
  if (!frontend)
    fprintf(stderr, "ERROR: GAME_register_frontend is called with NULL handle!!!\n");
  else
  {
    cb = frontend->GameLib_RegisterMe(frontend->addonData);
    if (!cb)
      fprintf(stderr, "ERROR: GAME_register_frontend can't get callback table from frontend!!!\n");
  }
  return cb;
}

DLLEXPORT void GAME_unregister_me(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;
  return frontend->GameLib_UnRegisterMe(frontend->addonData, cb);
}

DLLEXPORT void GAME_close_game(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->CloseGame(frontend->addonData);
}

DLLEXPORT bool GAME_open_port(AddonCB* frontend, CB_GameLib* cb, unsigned int port, const char* addon_id, game_input_device_caps* device_caps)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->OpenPort(frontend->addonData, port, addon_id, device_caps);
}

DLLEXPORT void GAME_close_port(AddonCB* frontend, CB_GameLib* cb, unsigned int port)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->ClosePort(frontend->addonData, port);
}

DLLEXPORT void GAME_environment_set_rotation(AddonCB* frontend, CB_GameLib* cb, GAME_ROTATION rotation)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->EnvironmentSetRotation(frontend->addonData, rotation);
}

DLLEXPORT bool GAME_environment_get_overscan(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->EnvironmentGetOverscan(frontend->addonData);
}

DLLEXPORT bool GAME_environment_can_dupe(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->EnvironmentCanDupe(frontend->addonData);
}

DLLEXPORT bool GAME_environment_set_system_av_info(AddonCB* frontend, CB_GameLib* cb, const struct game_system_av_info* info)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->EnvironmentSetSystemAvInfo(frontend->addonData, info);
}

DLLEXPORT bool GAME_video_frame(AddonCB* frontend, CB_GameLib* cb, GAME_RENDER_FORMAT format, unsigned int width, unsigned int height, const uint8_t* data)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->VideoFrame(frontend->addonData, format, width, height, data);
}

DLLEXPORT unsigned int GAME_audio_frames(AddonCB* frontend, CB_GameLib* cb, GAME_AUDIO_FORMAT format, unsigned int frames, const uint8_t* data)
{
  if (frontend == NULL || cb == NULL)
    return 0;
  return cb->AudioFrames(frontend->addonData, format, frames, data);
}

DLLEXPORT bool GAME_rumble_set_state(AddonCB* frontend, CB_GameLib* cb, unsigned port, GAME_RUMBLE_EFFECT effect, uint16_t strength)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->RumbleSetState(frontend->addonData, port, effect, strength);
}

DLLEXPORT game_time_t GAME_perf_get_time_usec(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return 0;
  return cb->PerfGetTimeUsec(frontend->addonData);
}

DLLEXPORT game_perf_tick_t GAME_perf_get_counter(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return 0;
  return cb->PerfGetCounter(frontend->addonData);
}

DLLEXPORT uint64_t GAME_perf_get_cpu_features(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return 0;
  return cb->PerfGetCpuFeatures(frontend->addonData);
}

DLLEXPORT void GAME_perf_log(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->PerfLog(frontend->addonData);
}

DLLEXPORT void GAME_perf_register(AddonCB* frontend, CB_GameLib* cb, game_perf_counter *counter)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->PerfRegister(frontend->addonData, counter);
}

DLLEXPORT void GAME_perf_start(AddonCB* frontend, CB_GameLib* cb, game_perf_counter *counter)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->PerfStart(frontend->addonData, counter);
}

DLLEXPORT void GAME_perf_stop(AddonCB* frontend, CB_GameLib* cb, game_perf_counter *counter)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->PerfStop(frontend->addonData, counter);
}

DLLEXPORT void GAME_camera_set_info(AddonCB* frontend, CB_GameLib* cb, game_camera_info *camera_info)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->CameraSetInfo(frontend->addonData, camera_info);
}

DLLEXPORT bool GAME_camera_start(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->CameraStart(frontend->addonData);
}

DLLEXPORT void GAME_camera_stop(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->CameraStop(frontend->addonData);
}

DLLEXPORT bool GAME_location_start(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->LocationStart(frontend->addonData);
}

DLLEXPORT void GAME_location_stop(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->LocationStop(frontend->addonData);
}

DLLEXPORT bool GAME_location_get_position(AddonCB* frontend, CB_GameLib* cb, double *lat, double *lon, double *horiz_accuracy, double *vert_accuracy)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->LocationGetPosition(frontend->addonData, lat, lon, horiz_accuracy, vert_accuracy);
}

DLLEXPORT void GAME_location_set_interval(AddonCB* frontend, CB_GameLib* cb, unsigned interval_ms, unsigned interval_distance)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->LocationSetInterval(frontend->addonData, interval_ms, interval_distance);
}

DLLEXPORT void GAME_location_initialized(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->LocationInitialized(frontend->addonData);
}

DLLEXPORT void GAME_location_deinitialized(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->LocationDeinitialized(frontend->addonData);
}

DLLEXPORT void GAME_frame_time_set_reference(AddonCB* frontend, CB_GameLib* cb, game_usec_t usec)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->FrameTimeSetReference(frontend->addonData, usec);
}

DLLEXPORT void GAME_hw_set_info(AddonCB* frontend, CB_GameLib* cb, game_hw_info *hw_info)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->HwSetInfo(frontend->addonData, hw_info);
}

DLLEXPORT uintptr_t GAME_hw_get_current_framebuffer(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return 0;
  return cb->HwGetCurrentFramebuffer(frontend->addonData);
}

DLLEXPORT game_proc_address_t GAME_hw_get_proc_address(AddonCB* frontend, CB_GameLib* cb, const char *sym)
{
  if (frontend == NULL || cb == NULL)
    return NULL;
  return cb->HwGetProcAddress(frontend->addonData, sym);
}

#ifdef __cplusplus
}
#endif
