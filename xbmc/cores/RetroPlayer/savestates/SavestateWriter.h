/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SavestateTypes.h"
#include "XBDateTime.h"

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
class CSavestateDatabase;

struct SavestateWritePayload
{
  std::string savePath;
  std::string gamePath;

  SAVE_TYPE type{SAVE_TYPE::UNKNOWN};
  uint8_t slot{0};
  std::string label;
  std::string caption;
  CDateTime created;
  std::string gameFileName;
  uint64_t timestampFrames{0};
  double timestampWallClock{0.0};
  std::string gameClientId;
  std::string gameClientVersion;

  AVPixelFormat pixelFormat{AV_PIX_FMT_NONE};
  unsigned int nominalWidth{0};
  unsigned int nominalHeight{0};
  float nominalDisplayAspectRatio{0.0f};
  unsigned int maxWidth{0};
  unsigned int maxHeight{0};

  std::vector<uint8_t> videoData;
  unsigned int videoWidth{0};
  unsigned int videoHeight{0};
  float displayAspectRatio{0.0f};
  unsigned int rotationCCW{0};

  std::vector<uint8_t> memoryData;
};

class CSavestateWriter
{
public:
  static bool WritePayload(const SavestateWritePayload& payload, CSavestateDatabase& database);
};
} // namespace RETRO
} // namespace KODI
