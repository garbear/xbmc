/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

namespace KODI
{
  namespace RETRO
  {
    enum class VariableType
    {
      ADDRESS, // Compare to the value of a live address in RAM
      INTEGER, // A constant integer
      DELTA_MEM, // The value last known at this address
    };

    enum class VariableSize
    {
      BIT_0,
      BIT_1,
      BIT_2,
      BIT_3,
      BIT_4,
      BIT_5,
      BIT_6,
      BIT_7,
      NIBBLE_LOWER,
      NIBBLE_UPPER,
      EIGHT_BITS,
      SIXTEEN_BITS,
      THIRTYTWO_BITS,
    };

    struct Variable
    {
      uint64_t address;
      unsigned int startBit;
      unsigned int endBit;
      bool bitwiseInvert;
      bool isBcd;
      unsigned int frameDelay;

      unsigned int Instantiate(const uint8_t* data, size_t size);
    };

    class CVariable
  }
  }
