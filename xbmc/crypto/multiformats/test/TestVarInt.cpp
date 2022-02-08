/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/multiformats/VarInt.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

TEST(TestVarInt, EncodingLength)
{
  ASSERT_EQ(VarInt::EncodingLength(0), 1);
  ASSERT_EQ(VarInt::EncodingLength(127), 1);
  ASSERT_EQ(VarInt::EncodingLength(128), 2);
  ASSERT_EQ(VarInt::EncodingLength(16383), 2);
  ASSERT_EQ(VarInt::EncodingLength(16384), 3);
}

TEST(TestVarInt, SingleByteVarints)
{
  uint8_t data[2];
  uint64_t result;

  // Test encoding
  ASSERT_TRUE(VarInt::EncodeTo(63, data, 2));
  ASSERT_EQ(data[0], 63);

  // Test decoding
  ASSERT_TRUE(VarInt::Decode(data, 2, result));
  ASSERT_EQ(result, 63);

  // Test larger single byte varint
  ASSERT_TRUE(VarInt::EncodeTo(127, data, 2));
  ASSERT_EQ(data[0], 127);
  ASSERT_TRUE(VarInt::Decode(data, 2, result));
  ASSERT_EQ(result, 127);
}

TEST(TestVarInt, MultiByteVarints)
{
  uint8_t data[5];
  uint64_t result;

  // Test two-byte varint encoding for value 300
  ASSERT_TRUE(VarInt::EncodeTo(300, data, 5));
  ASSERT_EQ(data[0], 0xAC);
  ASSERT_EQ(data[1], 0x02);

  // Test decoding for value 300
  ASSERT_TRUE(VarInt::Decode(data, 5, result));
  ASSERT_EQ(result, 300);

  // Test three-byte varint encoding for value 20000
  ASSERT_TRUE(VarInt::EncodeTo(20000, data, 5));
  ASSERT_EQ(data[0], 0xA0);
  ASSERT_EQ(data[1], 0x9C);
  ASSERT_EQ(data[2], 0x01);

  // Test decoding for value 20000
  ASSERT_TRUE(VarInt::Decode(data, 5, result));
  ASSERT_EQ(result, 20000);
}

TEST(TestVarInt, EdgeCases)
{
  uint8_t data[10];
  uint64_t result;

  // Largest 64-bit value
  constexpr uint64_t maxValue = UINT64_MAX;
  size_t length = VarInt::EncodingLength(maxValue);
  ASSERT_EQ(length, 10);
  ASSERT_TRUE(VarInt::EncodeTo(maxValue, data, sizeof(data)));
  ASSERT_TRUE(VarInt::Decode(data, sizeof(data), result));
  ASSERT_EQ(result, maxValue);

  // Decoding malformed varint (infinite sequence of bytes with the MSB set)
  uint8_t malformedData[10];

  // Filling the array with 0xFF
  std::fill(malformedData, malformedData + 10, 0xFF);

  ASSERT_FALSE(VarInt::Decode(malformedData, sizeof(malformedData), result));
}

TEST(TestVarInt, BufferOverflows)
{
  uint8_t data[1];
  uint64_t result;

  ASSERT_FALSE(VarInt::EncodeTo(300, data, 1));

  // Decoding with insufficient buffer
  uint8_t insufficientData[1] = {0xAC};
  ASSERT_FALSE(VarInt::Decode(insufficientData, 1, result));
}
