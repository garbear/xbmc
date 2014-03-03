/*
 *      Copyright (C) 2016 Team Kodi
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

#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"
#include "input/Key.h"

#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"

namespace GAME
{
  class CGameClientTranslator
  {
  public:
    static const char*   ToString(GAME_ERROR error);
    static AVPixelFormat TranslatePixelFormat(GAME_PIXEL_FORMAT format);
    static AVCodecID     TranslateVideoCodec(GAME_VIDEO_CODEC codec);
    static AEDataFormat  TranslatePCMFormat(GAME_PCM_FORMAT format);
    static AEChannel     TranslateAudioChannel(GAME_AUDIO_CHANNEL channel);
    static AVCodecID     TranslateAudioCodec(GAME_AUDIO_CODEC codec);
    static GAME_KEY_MOD  GetModifiers(CKey::Modifier modifier);
    static const char*   TranslateRegion(GAME_REGION region);
    
  };
}
