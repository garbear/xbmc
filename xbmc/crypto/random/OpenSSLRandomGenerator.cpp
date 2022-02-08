/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OpenSSLRandomGenerator.h"

#include <openssl/rand.h>

using namespace KODI;
using namespace CRYPTO;

COpenSSLRandomGenerator::~COpenSSLRandomGenerator() = default;

std::vector<uint8_t> COpenSSLRandomGenerator::RandomBytes(size_t length)
{
  std::vector<uint8_t> buffer(length, 0);

  //! @todo How safe is RAND_bytes?
  const int result = RAND_bytes(buffer.data(), buffer.size());

  // Verify result
  if (result != 1)
    buffer.clear();

  return buffer;
}
