/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace KODI
{
namespace RETRO
{
  class CVariable;

  class CTerm
  {
  public:
    virtual ~CTerm() = 0;

    bool invertBit = false;
    std::shared_ptr<CVariable> variable;
  };
}
}
