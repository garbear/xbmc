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

#include "AddonCallbacksGame.h"
#include "cores/RetroPlayer/RetroPlayer.h"
#include "games/addons/GameClient.h"
#include "utils/log.h"

#include <string>

using namespace GAME;

namespace ADDON
{

CAddonCallbacksGame::CAddonCallbacksGame(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_GameLib;

  /* write XBMC game specific add-on function addresses to callback table */
  m_callbacks->CloseGame                      = CloseGame;
  m_callbacks->VideoFrame                     = VideoFrame;
  m_callbacks->VideoFrameH264                 = VideoFrameH264;
  m_callbacks->AudioFrames                    = AudioFrames;
  m_callbacks->HwSetInfo                      = HwSetInfo;
  m_callbacks->HwGetCurrentFramebuffer        = HwGetCurrentFramebuffer;
  m_callbacks->HwGetProcAddress               = HwGetProcAddress;
  m_callbacks->OpenPort                       = OpenPort;
  m_callbacks->ClosePort                      = ClosePort;
  m_callbacks->RumbleSetState                 = RumbleSetState;
}

CAddonCallbacksGame::~CAddonCallbacksGame()
{
  /* delete the callback table */
  delete m_callbacks;
}

CGameClient* CAddonCallbacksGame::GetGameClient(void* addonData, const char* strFunction)
{
  CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
  if (!addon || !addon->GetHelperGame())
  {
    CLog::Log(LOGERROR, "GAME - %s - called with a null pointer", strFunction);
    return NULL;
  }

  return dynamic_cast<CGameClient*>(addon->GetHelperGame()->m_addon);
}

CRetroPlayer* CAddonCallbacksGame::GetRetroPlayer(void* addonData, const char* strFunction)
{
  CGameClient* gameClient = GetGameClient(addonData, strFunction);
  if (!gameClient)
  {
    CLog::Log(LOGERROR, "GAME - %s - invalid handler data", strFunction);
    return NULL;
  }

  if (!gameClient->GetPlayer())
  {
    CLog::Log(LOGERROR, "GAME - %s - game client is not playing a game", strFunction);
    return NULL;
  }

  CRetroPlayer* retroPlayer = dynamic_cast<CRetroPlayer*>(gameClient->GetPlayer());
  if (!retroPlayer)
  {
    CLog::Log(LOGERROR, "GAME - %s - active player is not RetroPlayer!", strFunction);
    return NULL;
  }

  return retroPlayer;
}

void CAddonCallbacksGame::CloseGame(void* addonData)
{
  CRetroPlayer* retroPlayer = GetRetroPlayer(addonData, __FUNCTION__);
  if (!retroPlayer)
    return;

  retroPlayer->CloseFile();
}

void CAddonCallbacksGame::VideoFrame(void* addonData, const uint8_t* data, unsigned int size, unsigned int width, unsigned int height, GAME_RENDER_FORMAT format)
{
  CRetroPlayer* retroPlayer = GetRetroPlayer(addonData, __FUNCTION__);
  if (!retroPlayer)
    return;

  AVPixelFormat pixelFormat(AV_PIX_FMT_NONE);

  switch (format)
  {
    case GAME_RENDER_FMT_YUV420P:
      pixelFormat = AV_PIX_FMT_YUV420P;
      break;
    case GAME_RENDER_FMT_0RGB8888:
      pixelFormat = AV_PIX_FMT_0RGB32;
      break;
    case GAME_RENDER_FMT_RGB565:
      pixelFormat = AV_PIX_FMT_RGB565;
      break;
    case GAME_RENDER_FMT_0RGB1555:
      pixelFormat = AV_PIX_FMT_RGB555;
      break;
    case GAME_RENDER_FMT_NONE:
    default:
      break;
  }

  if (pixelFormat == AV_PIX_FMT_NONE || width == 0 || height == 0 || data == NULL)
    return;

  retroPlayer->VideoFrame(data, size, width, height, pixelFormat);
}

bool CAddonCallbacksGame::VideoFrameH264(void* addonData, const uint8_t* data, unsigned int size, unsigned int width, unsigned int height)
{
  CRetroPlayer* retroPlayer = GetRetroPlayer(addonData, __FUNCTION__);
  if (!retroPlayer)
    return false;

  return retroPlayer->VideoFrameH264(data, size, width, height);
}

void CAddonCallbacksGame::AudioFrames(void* addonData, const uint8_t* data, unsigned int size, unsigned int frames, GAME_AUDIO_FORMAT format)
{
  CRetroPlayer* retroPlayer = GetRetroPlayer(addonData, __FUNCTION__);
  if (!retroPlayer)
    return;

  AEDataFormat audioFormat(AE_FMT_INVALID);

  switch (format)
  {
    case GAME_AUDIO_FMT_S16NE:
      audioFormat = AE_FMT_S16NE;
      break;
    case GAME_RENDER_FMT_NONE:
    default:
      break;
  }

  if (audioFormat == AE_FMT_INVALID || frames == 0 || data == NULL)
    return;

  retroPlayer->AudioFrames(data, size, frames, audioFormat);
}

void CAddonCallbacksGame::HwSetInfo(void* addonData, const game_hw_info *hw_info)
{
  // TODO
}

uintptr_t CAddonCallbacksGame::HwGetCurrentFramebuffer(void* addonData)
{
  // TODO
  return 0;
}

game_proc_address_t CAddonCallbacksGame::HwGetProcAddress(void* addonData, const char *sym)
{
  // TODO
  return NULL;
}

bool CAddonCallbacksGame::OpenPort(void* addonData, unsigned int port)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return false;

  return gameClient->OpenPort(port);
}

void CAddonCallbacksGame::ClosePort(void* addonData, unsigned int port)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return;

  gameClient->ClosePort(port);
}

void CAddonCallbacksGame::RumbleSetState(void* addonData, unsigned int port, GAME_RUMBLE_EFFECT effect, float strength)
{
  CRetroPlayer* retroPlayer = GetRetroPlayer(addonData, __FUNCTION__);
  if (!retroPlayer)
    return;

  // TODO
}

}; /* namespace ADDON */
