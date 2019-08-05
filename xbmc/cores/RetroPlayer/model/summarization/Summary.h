/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace RETRO
{
  class CConcept;
  class CPhrase;

  class CSummary
  {
  public:
    CSummary() = default;

    std::vector<std::shared_ptr<CConcept>> concepts;
    std::string license;
    std::vector<std::shared_ptr<CPhrase>> phrases;
  };
}
}
