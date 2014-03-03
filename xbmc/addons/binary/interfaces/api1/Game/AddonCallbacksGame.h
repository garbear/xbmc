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

#include "addons/binary/interfaces/AddonInterfaces.h"

namespace GAME { class CGameClient; }

namespace V1
{
namespace KodiAPI
{

namespace Game
{

/*!
 * Callbacks for a game add-on to Kodi
 */
class CAddonCallbacksGame : public ADDON::IAddonInterface
{
public:
  CAddonCallbacksGame(ADDON::CAddon* addon);
  ~CAddonCallbacksGame(void);

  /*!
   * @return The callback table.
   */
  CB_GameLib* GetCallbacks() const { return m_callbacks; }

  static void CloseGame(void* addonData);
  static int OpenVideoStream(void* addonData, GAME_VIDEO_FORMAT format, unsigned int width, unsigned int height);
  static void AddVideoData(void* addonData, const uint8_t* data, unsigned int size);
  static void CloseVideoStream(void* addonData);
  static int OpenAudioStream(void* addonData, GAME_AUDIO_FORMAT format, unsigned int samplerate, GAME_AUDIO_CHANNEL_LAYOUT channels);
  static void AddAudioData(void* addonData, const uint8_t* data, unsigned int size);
  static void CloseAudioStream(void* addonData);
  static void HwSetInfo(void* addonData, const game_hw_info* hw_info);
  static uintptr_t HwGetCurrentFramebuffer(void* addonData);
  static game_proc_address_t HwGetProcAddress(void* addonData, const char* sym);
  static bool OpenPort(void* addonData, unsigned int port);
  static void ClosePort(void* addonData, unsigned int port);
  static void RumbleSetState(void* addonData, unsigned int port, GAME_RUMBLE_EFFECT effect, float strength);

private:
  static GAME::CGameClient* GetGameClient(void* addonData, const char* strFunction);

  CB_GameLib*  m_callbacks; /*!< callback addresses */
};

} /* namespace Game */

} /* namespace KoidAPI */
} /* namespace V1 */
