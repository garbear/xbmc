/*
 *      Copyright (C) 2018 Team Kodi
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

#include "GameClientStreamHwFramebuffer.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "cores/RetroPlayer/streams/RetroPlayerRendering.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CGameClientStreamHwFramebuffer::CGameClientStreamHwFramebuffer(IHwFramebufferCallback& callback) :
  m_callback(callback)
{
}

bool CGameClientStreamHwFramebuffer::OpenStream(RETRO::IRetroPlayerStream* stream, const game_stream_properties& properties)
{
  RETRO::CRetroPlayerRendering* renderingStream = dynamic_cast<RETRO::CRetroPlayerRendering*>(stream);
  if (renderingStream == nullptr)
  {
    CLog::Log(LOGERROR, "GAME: RetroPlayer stream is not an audio stream");
    return false;
  }

  RETRO::StreamProperties renderingProperties{};

  if (stream->OpenStream(renderingProperties))
  {
    m_stream = stream;
    m_callback.HardwareContextReset();
  }

  return m_stream != nullptr;
}

void CGameClientStreamHwFramebuffer::CloseStream()
{
  m_stream->CloseStream();
  m_stream = nullptr;
}

bool CGameClientStreamHwFramebuffer::GetBuffer(unsigned int width, unsigned int height, game_stream_buffer& buffer)
{
  if (buffer.type != GAME_STREAM_HW_FRAMEBUFFER)
    return false;

  if (m_stream != nullptr)
  {
    RETRO::HwFramebufferBuffer hwFramebufferBuffer;
    if (m_stream->GetStreamBuffer(0, 0, static_cast<RETRO::StreamBuffer&>(hwFramebufferBuffer)))
    {
      buffer.hw_framebuffer.framebuffer = hwFramebufferBuffer.framebuffer;
      return true;
    }
  }

  return false;
}

void CGameClientStreamHwFramebuffer::AddData(const game_stream_packet &packet)
{
  if (packet.type != GAME_STREAM_HW_FRAMEBUFFER)
    return;

  if (m_stream != nullptr)
  {
    const game_stream_hw_framebuffer_packet &hwFramebuffer = packet.hw_framebuffer;

    RETRO::HwFramebufferPacket hwFramebufferPacket{
      hwFramebuffer.framebuffer
    };

    m_stream->AddStreamData(static_cast<const RETRO::StreamPacket&>(hwFramebufferPacket));
  }
}
