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

namespace KODI
{
namespace RETRO
{
  class COperation;
  class CVariable;

  class CCondition
  {
  public:
    CCondition() = default;

    std::string conditionType;
    unsigned int requiredHits = 0;
    std::shared_ptr<COperation> operation;
    std::shared_ptr<CVariable> sourceVariable;
    std::shared_ptr<CVariable> targetVariable;
  };
}
}
