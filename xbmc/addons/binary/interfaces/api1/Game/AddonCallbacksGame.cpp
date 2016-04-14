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

#include "AddonCallbacksGame.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "games/addons/GameClient.h"
#include "utils/log.h"

//#include "libavutil/avconfig.h"

#include <string>

using namespace ADDON;
using namespace GAME;

namespace V1
{
namespace KodiAPI
{

namespace Game
{

CAddonCallbacksGame::CAddonCallbacksGame(CAddon* addon) :
  ADDON::IAddonInterface(addon, 1, GAME_API_VERSION),
  m_callbacks(new CB_GameLib)
{
  /* write Kodi game specific add-on function addresses to callback table */
  m_callbacks->CloseGame                      = CloseGame;
  m_callbacks->OpenVideoStream                = OpenVideoStream;
  m_callbacks->AddVideoData                   = AddVideoData;
  m_callbacks->CloseVideoStream               = CloseVideoStream;
  m_callbacks->OpenAudioStream                = OpenAudioStream;
  m_callbacks->AddAudioData                   = AddAudioData;
  m_callbacks->CloseAudioStream               = CloseAudioStream;
  m_callbacks->HwSetInfo                      = HwSetInfo;
  m_callbacks->HwGetCurrentFramebuffer        = HwGetCurrentFramebuffer;
  m_callbacks->HwGetProcAddress               = HwGetProcAddress;
  m_callbacks->OpenPort                       = OpenPort;
  m_callbacks->ClosePort                      = ClosePort;
  m_callbacks->InputEvent                     = InputEvent;
}

CAddonCallbacksGame::~CAddonCallbacksGame()
{
  /* delete the callback table */
  delete m_callbacks;
}

CGameClient* CAddonCallbacksGame::GetGameClient(void* addonData, const char* strFunction)
{
  CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(addonData);
  if (!addon || !addon->GetHelperGame())
  {
    CLog::Log(LOGERROR, "GAME - %s - called with a null pointer", strFunction);
    return NULL;
  }

  return dynamic_cast<CGameClient*>(static_cast<CAddonCallbacksGame*>(addon->GetHelperGame())->m_addon);
}

void CAddonCallbacksGame::CloseGame(void* addonData)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return;

  return gameClient->CloseFile();
}

int CAddonCallbacksGame::OpenVideoStream(void* addonData, GAME_VIDEO_FORMAT format, unsigned int width, unsigned int height)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return -1;

  AVCodecID codec = AV_CODEC_ID_NONE;
  AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;

  switch (format)
  {
    case GAME_VIDEO_FORMAT_YUV420P:
      pixelFormat = AV_PIX_FMT_YUV420P;
      break;
    case GAME_VIDEO_FORMAT_0RGB8888:
      pixelFormat = AV_PIX_FMT_0RGB32;
      break;
    case GAME_VIDEO_FORMAT_RGB565:
      pixelFormat = AV_PIX_FMT_RGB565;
      break;
    case GAME_VIDEO_FORMAT_0RGB1555:
      pixelFormat = AV_PIX_FMT_RGB555;
      break;
    case GAME_VIDEO_FORMAT_H264:
      codec = AV_CODEC_ID_H264;
      break;
    case GAME_VIDEO_FORMAT_UNKNOWN:
    default:
      break;
  }

  if (codec == AV_CODEC_ID_NONE && pixelFormat == AV_PIX_FMT_NONE)
    return -1;

  return gameClient->OpenVideoStream(codec, pixelFormat, width, height) ? 0 : -1;
}

void CAddonCallbacksGame::AddVideoData(void* addonData, const uint8_t* data, unsigned int size)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return;

  gameClient->AddVideoData(data, size);
}

void CAddonCallbacksGame::CloseVideoStream(void* addonData)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return;

  gameClient->CloseVideoStream();
}

int CAddonCallbacksGame::OpenAudioStream(void* addonData, GAME_AUDIO_FORMAT format, unsigned int samplerate, GAME_AUDIO_CHANNEL_LAYOUT channels)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return -1;

  AVCodecID codec = AV_CODEC_ID_NONE;
  CAEChannelInfo channelLayout;

  switch (format)
  {
    case GAME_AUDIO_FORMAT_S16NE:
#if AV_HAVE_BIGENDIAN
      codec = AV_CODEC_ID_PCM_S16BE;
#else
      codec = AV_CODEC_ID_PCM_S16LE;
#endif
      break;
    case GAME_AUDIO_FORMAT_OPUS:
      codec = AV_CODEC_ID_OPUS;
      break;
    case GAME_AUDIO_FORMAT_UNKNOWN:
    default:
      break;
  }

  switch (channels)
  {
    case CH_FL_FR:
    {
      static enum AEChannel layout[] = {AE_CH_FL, AE_CH_FR, AE_CH_NULL};
      channelLayout = layout;
      break;
    }
    case CH_FL_C_FR:
    {
      static enum AEChannel layout[] = {AE_CH_FL, AE_CH_FC, AE_CH_FR, AE_CH_NULL};
      channelLayout = layout;
      break;
    }
    case CH_FL_FR_RL_RR:
    {
      static enum AEChannel layout[] = {AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_NULL};
      channelLayout = layout;
      break;
    }
    case CH_FL_C_FR_RL_RR_LFE:
    {
      static enum AEChannel layout[] = {AE_CH_FL, AE_CH_FC, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_LFE, AE_CH_NULL};
      channelLayout = layout;
      break;
    }
    case GAME_AUDIO_CHANNEL_UNKNOWN:
    default:
      break;
  }

  if (codec == AV_CODEC_ID_NONE)
    return -1;

  return gameClient->OpenAudioStream(codec, samplerate, channelLayout) ? 0 : -1;
}

void CAddonCallbacksGame::AddAudioData(void* addonData, const uint8_t* data, unsigned int size)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return;

  gameClient->AddAudioData(data, size);
}

void CAddonCallbacksGame::CloseAudioStream(void* addonData)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return;

  gameClient->CloseAudioStream();
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
  return nullptr;
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

bool CAddonCallbacksGame::InputEvent(void* addonData, const game_input_event* event)
{
  CGameClient* gameClient = GetGameClient(addonData, __FUNCTION__);
  if (!gameClient)
    return false;

  if (event == nullptr)
    return false;

  return gameClient->ReceiveInputEvent(*event);
}

} /* namespace Game */

} /* namespace KodiAPI */
} /* namespace V1 */
