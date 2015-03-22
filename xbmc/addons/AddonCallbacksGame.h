#pragma once
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

#include "AddonCallbacks.h"
#include "include/kodi_game_callbacks.h"

namespace GAME { class CGameClient; }
class CRetroPlayer;

namespace ADDON
{

/*!
 * Callbacks for a game add-on to XBMC.
 *
 * Also translates the addon's C structures to XBMC's C++ structures.
 */
class CAddonCallbacksGame
{
public:
  CAddonCallbacksGame(CAddon* addon);
  ~CAddonCallbacksGame(void);

  /*!
   * @return The callback table.
   */
  CB_GameLib* GetCallbacks() const { return m_callbacks; }

  static void CloseGame(void* addonData);
  static bool OpenPort(void* addonData, unsigned int port, const char* addon_id, game_input_device_caps* device_caps);
  static void ClosePort(void* addonData, unsigned int port);
  static void EnvironmentSetRotation(void* addonData, GAME_ROTATION rotation);
  static bool EnvironmentGetOverscan(void* addonData);
  static bool EnvironmentCanDupe(void* addonData);
  static bool EnvironmentSetSystemAvInfo(void* addonData, const game_system_av_info* info);
  static bool VideoFrame(void* addonData, GAME_RENDER_FORMAT format, unsigned int width, unsigned int height, const uint8_t* data);
  static unsigned int AudioFrames(void* addonData, GAME_AUDIO_FORMAT format, unsigned int frames, const uint8_t* data);
  static bool RumbleSetState(void* addonData, unsigned port, GAME_RUMBLE_EFFECT effect, uint16_t strength);
  static game_time_t PerfGetTimeUsec(void* addonData);
  static game_perf_tick_t PerfGetCounter(void* addonData);
  static uint64_t PerfGetCpuFeatures(void* addonData);
  static void PerfLog(void* addonData);
  static void PerfRegister(void* addonData, game_perf_counter *counter);
  static void PerfStart(void* addonData, game_perf_counter *counter);
  static void PerfStop(void* addonData, game_perf_counter *counter);
  static void CameraSetInfo(void* addonData, game_camera_info *camera_info);
  static bool CameraStart(void* addonData);
  static void CameraStop(void* addonData);
  static bool LocationStart(void* addonData);
  static void LocationStop(void* addonData);
  static bool LocationGetPosition(void* addonData, double *lat, double *lon, double *horiz_accuracy, double *vert_accuracy);
  static void LocationSetInterval(void* addonData, unsigned interval_ms, unsigned interval_distance);
  static void LocationInitialized(void* addonData);
  static void LocationDeinitialized(void* addonData);
  static void FrameTimeSetReference(void* addonData, game_usec_t usec);
  static void HwSetInfo(void* addonData, const game_hw_info *hw_info);
  static uintptr_t HwGetCurrentFramebuffer(void* addonData);
  static game_proc_address_t HwGetProcAddress(void* addonData, const char *sym);

private:
  static GAME::CGameClient* GetGameClient(void* addonData, const char* strFunction);
  static CRetroPlayer* GetRetroPlayer(void* addonData, const char* strFunction);

  CB_GameLib*  m_callbacks; /*!< callback addresses */
  CAddon*      m_addon;     /*!< the addon */
};

}; /* namespace ADDON */
