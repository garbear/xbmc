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
#pragma once

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
  static void VideoFrame(void* addonData, const uint8_t* data, unsigned int size, unsigned int width, unsigned int height, GAME_RENDER_FORMAT format);
  static bool VideoFrameH264(void* addonData, const uint8_t* data, unsigned int size, unsigned int width, unsigned int height);
  static void AudioFrames(void* addonData, const uint8_t* data, unsigned int size, unsigned int frames, GAME_AUDIO_FORMAT format);
  static void HwSetInfo(void* addonData, const game_hw_info* hw_info);
  static uintptr_t HwGetCurrentFramebuffer(void* addonData);
  static game_proc_address_t HwGetProcAddress(void* addonData, const char* sym);
  static bool OpenPort(void* addonData, unsigned int port);
  static void ClosePort(void* addonData, unsigned int port);
  static void RumbleSetState(void* addonData, unsigned int port, GAME_RUMBLE_EFFECT effect, float strength);

private:
  static GAME::CGameClient* GetGameClient(void* addonData, const char* strFunction);
  static CRetroPlayer* GetRetroPlayer(void* addonData, const char* strFunction);

  CB_GameLib*  m_callbacks; /*!< callback addresses */
  CAddon*      m_addon;     /*!< the addon */
};

}; /* namespace ADDON */
