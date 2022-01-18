/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroEngineStreamSwFramebuffer.h"

#include "cores/RetroPlayer/streams/RetroPlayerVideo.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO_ENGINE;

bool CRetroEngineStreamSwFramebuffer::OpenStream(RETRO::IRetroPlayerStream* stream,
                                                 AVPixelFormat nominalFormat,
                                                 unsigned int nominalWidth,
                                                 unsigned int nominalHeight)
{
  RETRO::CRetroPlayerVideo* videoStream = dynamic_cast<RETRO::CRetroPlayerVideo*>(stream);
  if (videoStream == nullptr)
  {
    CLog::Log(LOGERROR, "GAME: RetroPlayer stream is not a video stream");
    return false;
  }

  const float pixelAspectRatio =
      static_cast<float>(nominalWidth) / static_cast<float>(nominalHeight);
  RETRO::VideoStreamProperties videoProperties{nominalFormat, nominalWidth,  nominalHeight,
                                               nominalWidth,  nominalHeight, pixelAspectRatio};

  if (videoStream->OpenStream(static_cast<const RETRO::StreamProperties&>(videoProperties)))
    m_stream = stream;

  return m_stream != nullptr;
}

void CRetroEngineStreamSwFramebuffer::CloseStream()
{
  if (m_stream != nullptr)
  {
    m_stream->CloseStream();
    m_stream = nullptr;
  }
}

bool CRetroEngineStreamSwFramebuffer::GetBuffer(
    unsigned int width, unsigned int height, AVPixelFormat& format, uint8_t*& data, size_t& size)
{
  if (m_stream != nullptr)
  {
    RETRO::VideoStreamBuffer streamBuffer;
    if (m_stream->GetStreamBuffer(width, height, static_cast<RETRO::StreamBuffer&>(streamBuffer)))
    {
      format = streamBuffer.pixfmt;
      data = streamBuffer.data;
      size = streamBuffer.size;

      return true;
    }
  }

  return false;
}

void CRetroEngineStreamSwFramebuffer::AddData(unsigned int width,
                                              unsigned int height,
                                              const uint8_t* data,
                                              size_t size)
{
  if (m_stream != nullptr)
  {
    const RETRO::VideoRotation rotation = RETRO::VideoRotation::ROTATION_0;

    RETRO::VideoStreamPacket videoPacket{width, height, rotation, data, size};

    m_stream->AddStreamData(static_cast<const RETRO::StreamPacket&>(videoPacket));
  }
}
