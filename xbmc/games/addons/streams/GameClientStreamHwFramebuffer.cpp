/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreamHwFramebuffer.h"

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "cores/RetroPlayer/streams/RetroPlayerRendering.h"
#include "games/addons/GameClientTranslator.h"
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
    CLog::Log(LOGERROR, "GAME: RetroPlayer stream is not an rendering stream");
    return false;
  }

  std::unique_ptr<RETRO::StreamProperties> renderingProperties(
      TranslateProperties(properties.video));
  if (renderingProperties)
  {
    if (stream->OpenStream(static_cast<const RETRO::StreamProperties&>(*renderingProperties)))
      m_stream = stream;
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
    if (m_stream->GetStreamBuffer(width, height,
                                  static_cast<RETRO::StreamBuffer&>(hwFramebufferBuffer)))
    {
      buffer.hw_framebuffer.framebuffer = hwFramebufferBuffer.framebuffer;

      if (!m_contextReset)
      {
        m_callback.HardwareContextReset();
        m_contextReset = true;
      }

      return true;
    }
  }

  return false;
}

void CGameClientStreamHwFramebuffer::AddData(const game_stream_packet& packet)
{
  if (packet.type != GAME_STREAM_HW_FRAMEBUFFER)
    return;

  if (m_stream != nullptr)
  {
    // This is left here in case anything gets added to the api in the future
    [[maybe_unused]] const game_stream_hw_framebuffer_packet& hwFramebuffer = packet.hw_framebuffer;

    RETRO::HwFramebufferPacket hwFramebufferPacket{};

    m_stream->AddStreamData(static_cast<const RETRO::StreamPacket&>(hwFramebufferPacket));
  }
}

RETRO::RenderingStreamProperties* CGameClientStreamHwFramebuffer::TranslateProperties(
    const game_stream_video_properties& properties)
{
  const AVPixelFormat pixelFormat = CGameClientTranslator::TranslatePixelFormat(properties.format);
  if (pixelFormat == AV_PIX_FMT_NONE)
  {
    CLog::Log(LOGERROR, "GAME: Unknown pixel format: {}", properties.format);
    return nullptr;
  }

  const unsigned int nominalWidth = properties.nominal_width;
  const unsigned int nominalHeight = properties.nominal_height;
  if (nominalWidth == 0 || nominalHeight == 0)
  {
    CLog::Log(LOGERROR, "GAME: Invalid nominal dimensions: {}x{}", nominalWidth, nominalHeight);
    return nullptr;
  }

  const unsigned int maxWidth = properties.max_width;
  const unsigned int maxHeight = properties.max_height;
  if (maxWidth == 0 || maxHeight == 0)
  {
    CLog::Log(LOGERROR, "GAME: Invalid max dimensions: {}x{}", maxWidth, maxHeight);
    return nullptr;
  }

  if (nominalWidth > maxWidth || nominalHeight > maxHeight)
    CLog::Log(LOGERROR, "GAME: Nominal dimensions ({}x{}) bigger than max dimensions ({}x{})",
              nominalWidth, nominalHeight, maxWidth, maxHeight);

  float pixelAspectRatio;

  // Game API: If aspect_ratio is <= 0.0, an aspect ratio of
  // (nominal_width / nominal_height) is assumed
  if (properties.aspect_ratio <= 0.0f)
    pixelAspectRatio = 1.0f;
  else
    pixelAspectRatio = properties.aspect_ratio * nominalHeight / nominalWidth;

  return new RETRO::RenderingStreamProperties{pixelFormat, nominalWidth, nominalHeight,
                                              maxWidth,    maxHeight,    pixelAspectRatio};
}
