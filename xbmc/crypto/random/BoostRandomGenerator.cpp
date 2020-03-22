/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BoostRandomGenerator.h"

using namespace KODI;
using namespace CRYPTO;

CBoostRandomGenerator::~CBoostRandomGenerator() = default;

std::vector<uint8_t> CBoostRandomGenerator::RandomBytes(size_t length)
{
  std::vector<uint8_t> buffer(length, 0);

  for (size_t i = 0; i < length; ++i)
  {
    unsigned char byte = m_distribution(m_generator);
    buffer[i] = byte;
  }

  return buffer;
}
