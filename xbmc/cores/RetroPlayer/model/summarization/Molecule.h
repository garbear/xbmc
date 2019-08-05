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
  class CExpression;

  class CMolecule
  {
  public:
    CMolecule() = default;

    std::shared_ptr<CExpression> expression;
    unsigned int index = 0;
    std::string symbol;
  };
}
}
