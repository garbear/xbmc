/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

namespace KODI
{
namespace RETRO
{
  enum class VideoFormat
  {
    FORMAT_0RGB8888,
    FORMAT_RGB565,
    FORMAT_0RGB1555,
  };

  struct VideoState
  {
    VideoFormat format;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int stride = 0;
    unsigned int orientationDeg = 0; // Counter clockwise
    uint8_t *m_pixels = nullptr;
  };

  class CState
  {
  public:
    //! @todo

  private:
    uint64_t m_timestamp = 0;
    VideoState m_video = {};
    //AudioState m_audio = {};
    //MemoryState m_memory = {};
    //FeedbackState m_feedback = {};
  };
}
}
