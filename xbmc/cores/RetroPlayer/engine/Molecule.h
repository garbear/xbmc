/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SummaryTypes.h"

#include <string>

namespace KODI
{
  namespace RETRO
  {
    struct Molecule
    {
      string characters;
      std::string symbol;
      ExpressionPtr expression;
    };
  }
}

