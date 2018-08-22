/*
 *  Copyright (C) 2018-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreamHwFramebuffer.h"

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "cores/RetroPlayer/streams/RetroPlayerRendering.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CGameClientStreamHwFramebuffer::CGameClientStreamHwFramebuffer(IHwFramebufferCallback& callback)
  : m_callback(callback)
{
}

bool CGameClientStreamHwFramebuffer::OpenStream(RETRO::IRetroPlayerStream* stream,
                                                const game_stream_properties& properties)
{
  RETRO::CRetroPlayerRendering* renderingStream =
      dynamic_cast<RETRO::CRetroPlayerRendering*>(stream);
  if (renderingStream == nullptr)
  {
    CLog::Log(LOGERROR, "GAME: RetroPlayer stream is not an audio stream");
    return false;
  }

  RETRO::StreamProperties renderingProperties{};

  if (stream->OpenStream(renderingProperties))
  {
    m_stream = stream;
    //TODO guess this needs to be called later
    //m_callback.HardwareContextReset();
  }

  return m_stream != nullptr;
}

void CGameClientStreamHwFramebuffer::CloseStream()
{
  if (m_stream != nullptr)
  {
    m_stream->CloseStream();
    m_stream = nullptr;
  }
}

bool CGameClientStreamHwFramebuffer::GetBuffer(unsigned int width,
                                               unsigned int height,
                                               game_stream_buffer& buffer)
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

void CGameClientStreamHwFramebuffer::AddData(const game_stream_packet& packet)
{
  if (packet.type != GAME_STREAM_HW_FRAMEBUFFER && packet.type != GAME_STREAM_HW_FRAMEBUFFER)
    return;

  if (m_stream != nullptr)
  {
    const game_stream_hw_framebuffer_packet& hwFramebuffer = packet.hw_framebuffer;

    RETRO::HwFramebufferPacket hwFramebufferPacket{hwFramebuffer.framebuffer};

    m_stream->AddStreamData(static_cast<const RETRO::StreamPacket&>(hwFramebufferPacket));
  }
}
