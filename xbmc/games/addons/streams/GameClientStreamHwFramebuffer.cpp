/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreamHwFramebuffer.h"

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "cores/RetroPlayer/streams/RetroPlayerVideo.h" // TODO

using namespace KODI;
using namespace GAME;

bool CGameClientStreamHwFramebuffer::OpenStream(RETRO::IRetroPlayerStream* stream,
                                                const game_stream_properties& properties)
{
  // TODO
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
  if (m_stream != nullptr)
  {
    // TODO
  }

  return false;
}

void CGameClientStreamHwFramebuffer::AddData(const game_stream_packet& packet)
{
  if (packet.type != GAME_STREAM_HW_FRAMEBUFFER && packet.type != GAME_STREAM_HW_FRAMEBUFFER)
    return;

  if (m_stream != nullptr)
  {
    const uintptr_t framebuffer = packet.hw_framebuffer.framebuffer;

    // TODO
    (void)framebuffer;
  }
}
