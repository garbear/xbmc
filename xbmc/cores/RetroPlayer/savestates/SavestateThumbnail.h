/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace KODI
{
namespace RETRO
{
struct SavestateThumbnailPayload
{
  std::string thumbnailPath;
  std::vector<uint8_t> pixels;
  unsigned int width{0};
  unsigned int height{0};
  unsigned int rotationCCW{0};
  AVPixelFormat pixelFormat{AV_PIX_FMT_NONE};
};
} // namespace RETRO
} // namespace KODI
