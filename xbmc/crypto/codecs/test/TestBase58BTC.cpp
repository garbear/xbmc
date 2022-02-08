/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/codecs/Base58BTC.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

TEST(TestBase58BTC, EncodeBase58)
{
  // Simple data
  uint8_t data[] = {0x61, 0x62, 0x63}; // Represents the string "abc"
  auto encoded = CBase58BTC::EncodeBase58(data, sizeof(data));
  EXPECT_EQ(encoded, "ZiCa");

  // Empty data
  std::vector<uint8_t> emptyDataVec;
  encoded = CBase58BTC::EncodeBase58(emptyDataVec.data(), emptyDataVec.size());
  EXPECT_EQ(encoded, "");
}

TEST(TestBase58BTC, DecodeBase58)
{
  // Valid encoded string
  std::string encoded = "ZiCa";
  std::vector<uint8_t> decoded;
  bool success = CBase58BTC::DecodeBase58(encoded, decoded);
  EXPECT_TRUE(success);
  EXPECT_EQ(decoded, std::vector<uint8_t>({0x61, 0x62, 0x63}));

  // Invalid encoded string
  encoded = "ZiIa"; // "I" is not in the base58 alphabet
  success = CBase58BTC::DecodeBase58(encoded, decoded);
  EXPECT_FALSE(success);

  // Empty string
  encoded = "";
  success = CBase58BTC::DecodeBase58(encoded, decoded);
  EXPECT_TRUE(success);
  EXPECT_TRUE(decoded.empty());
}

TEST(TestBase58BTC, EncodeBase58LeadingZeros)
{
  // Data with leading zeros
  uint8_t data[] = {0x00, 0x00, 0x61, 0x62, 0x63}; // Leading zeros with "abc"
  auto encoded = CBase58BTC::EncodeBase58(data, sizeof(data));
  EXPECT_EQ(encoded, "11ZiCa");
}

TEST(TestBase58BTC, DecodeBase58LeadingOnes)
{
  // Encoded string with leading '1's (representing zeros)
  std::string encoded = "11ZiCa";
  std::vector<uint8_t> decoded;
  bool success = CBase58BTC::DecodeBase58(encoded, decoded);
  EXPECT_TRUE(success);
  EXPECT_EQ(decoded, std::vector<uint8_t>({0x00, 0x00, 0x61, 0x62, 0x63}));
}

TEST(TestBase58BTC, EncodeLargeData)
{
  // Larger data set
  uint8_t data[1000];
  for (int i = 0; i < 1000; ++i)
  {
    data[i] = i % 256; // Fill with repeating byte patterns
  }
  auto encoded = CBase58BTC::EncodeBase58(data, sizeof(data));
  EXPECT_FALSE(encoded.empty()); // Just checking that encoding succeeds

  std::vector<uint8_t> decoded;
  bool success = CBase58BTC::DecodeBase58(encoded, decoded);
  EXPECT_TRUE(success);
  EXPECT_EQ(decoded.size(), sizeof(data));
  for (int i = 0; i < 1000; ++i)
  {
    EXPECT_EQ(decoded[i], i % 256);
  }
}

TEST(TestBase58BTC, DecodeInvalidChars)
{
  // Encoded string with characters not in base58
  std::string encoded = "Zi$#a";
  std::vector<uint8_t> decoded;
  bool success = CBase58BTC::DecodeBase58(encoded, decoded);
  EXPECT_FALSE(success);
}

TEST(TestBase58BTC, DecodeWithLeadingSpaces)
{
  // Valid encoded string with leading spaces (should be skipped)
  std::string encoded = "    ZiCa";
  std::vector<uint8_t> decoded;
  bool success = CBase58BTC::DecodeBase58(encoded, decoded);
  EXPECT_TRUE(success);
  EXPECT_EQ(decoded, std::vector<uint8_t>({0x61, 0x62, 0x63}));
}
