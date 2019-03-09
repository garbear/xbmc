/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Variant.h"

namespace KODI
{
  namespace RETRO
  {
    enum class VariableType
    {
      ADDRESS = 0, // Compare to the value of a live address in RAM
      INTEGER = 1, // A constant integer
      DELTA_MEM = 2, // The value last known at this address
    };

    enum class VariableSize
    {
      BIT_0 = 'M',
      BIT_1 = 'N',
      BIT_2 = 'O',
      BIT_3 = 'P',
      BIT_4 = 'Q',
      BIT_5 = 'R',
      BIT_6 = 'S',
      BIT_7 = 'T',
      NIBBLE_LOWER = 'L',
      NIBBLE_UPPER = 'U',
      EIGHT_BITS = 'H',
      SIXTEEN_BITS = ' ',
      THIRTYTWO_BITS = 'X',
    };

    struct Variable
    {
      CVariant address;
      CVariant startBit;
      CVariant endBit;
      bool isBcd = false;
      CVariant frameDelay;
    };
  }
  }
