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
  class COperand;

  class CPolyadicExpression
  {
  public:
    CPolyadicExpression() = default;

    std::string expressionOperator;
    std::vector<std::shared_ptr<COperand>> operands;
  };
}
}
