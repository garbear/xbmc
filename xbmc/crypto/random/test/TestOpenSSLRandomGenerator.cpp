/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/CryptoTypes.h"
#include "crypto/random/OpenSSLRandomGenerator.h"

#include <algorithm>

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

TEST(TestOpenSSLRandomGenerator, TestRandomBytes)
{
  // Create CSPRNG
  std::unique_ptr<IRandomGenerator> csprng(new COpenSSLRandomGenerator);
  ByteArray buffer = csprng->RandomBytes(32);

  // Check size
  EXPECT_EQ(buffer.size(), 32);

  // Check data
  ASSERT_TRUE(std::any_of(buffer.begin(), buffer.end(), [](uint8_t byte) { return byte != 0; }));
}
