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

#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"

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

DLLEXPORT int GAME_open_video_stream(AddonCB* frontend, CB_GameLib* cb, GAME_VIDEO_FORMAT format, unsigned int width, unsigned int height)
{
  if (frontend == NULL || cb == NULL)
    return -1;

  return cb->OpenVideoStream(frontend->addonData, format, width, height);
}

DLLEXPORT void GAME_add_video_data(AddonCB* frontend, CB_GameLib* cb, const uint8_t* data, unsigned int size)
{
  if (frontend == NULL || cb == NULL)
    return;

  return cb->AddVideoData(frontend->addonData, data, size);
}

DLLEXPORT void GAME_close_video_stream(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;

  return cb->CloseVideoStream(frontend->addonData);
}

DLLEXPORT int GAME_open_audio_stream(AddonCB* frontend, CB_GameLib* cb, GAME_AUDIO_FORMAT format, unsigned int samplerate, GAME_AUDIO_CHANNEL_LAYOUT channels)
{
  if (frontend == NULL || cb == NULL)
    return -1;

  return cb->OpenAudioStream(frontend->addonData, format, samplerate, channels);
}

DLLEXPORT void GAME_add_audio_data(AddonCB* frontend, CB_GameLib* cb, const uint8_t* data, unsigned int size)
{
  if (frontend == NULL || cb == NULL)
    return;

  return cb->AddAudioData(frontend->addonData, data, size);
}

DLLEXPORT void GAME_close_audio_stream(AddonCB* frontend, CB_GameLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;

  return cb->CloseAudioStream(frontend->addonData);
}

DLLEXPORT void GAME_hw_set_info(AddonCB* frontend, CB_GameLib* cb, game_hw_info* hw_info)
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

DLLEXPORT game_proc_address_t GAME_hw_get_proc_address(AddonCB* frontend, CB_GameLib* cb, const char* sym)
{
  if (frontend == NULL || cb == NULL)
    return NULL;
  return cb->HwGetProcAddress(frontend->addonData, sym);
}

DLLEXPORT bool GAME_open_port(AddonCB* frontend, CB_GameLib* cb, unsigned int port)
{
  if (frontend == NULL || cb == NULL)
    return false;
  return cb->OpenPort(frontend->addonData, port);
}

DLLEXPORT void GAME_close_port(AddonCB* frontend, CB_GameLib* cb, unsigned int port)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->ClosePort(frontend->addonData, port);
}

DLLEXPORT void GAME_rumble_set_state(AddonCB* frontend, CB_GameLib* cb, unsigned int port, GAME_RUMBLE_EFFECT effect, float strength)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->RumbleSetState(frontend->addonData, port, effect, strength);
}

#ifdef __cplusplus
}
#endif
