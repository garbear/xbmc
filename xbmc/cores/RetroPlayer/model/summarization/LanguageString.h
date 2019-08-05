/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace RETRO
{
  class CLanguageString
  {
  public:
    CLanguageString() = default;

    std::string language;
    std::string string;
  };
}
}
