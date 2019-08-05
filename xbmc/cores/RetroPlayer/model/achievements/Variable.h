/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace RETRO
{
  class CVariable
  {
  public:
    CVariable() = default;

    unsigned int address = 0;
    unsigned int startBit = 0;
    unsigned int endBit = 0;
    bool bcd = false;
    unsigned int frameDelay = 0;
  };
}
}
