/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <stdint.h>
#include <string>

namespace KODI
{
namespace RETRO
{
  class CCondition;
  class CLanguageString;

  class CAchievement
  {
  public:
    CAchievement() = default;

    std::string author;
    std::unique_ptr<CCondition> condition;
    uint64_t created = 0;
    std::string flags;
    std::string badgeUnlocked;
    std::string badgeLocked;
    uint64_t modified = 0;
    unsigned int points = 0;
    std::unique_ptr<CLanguageString> title;
  };
}
}
