/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
namespace UTILS
{

class CQRUtils
{
public:
  static bool EncodeString(const std::string& data, std::vector<bool>& pixels, unsigned int& width);
  static bool EncodeData(const std::vector<uint8_t>& data,
                         std::vector<bool>& pixels,
                         unsigned int& width);

  private:
  static void TranslateData(std::vector<bool>& pixels, const uint8_t* data, unsigned int width);
};

} // namespace UTILS
} // namespace KODI
