/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/multiformats/Multihash.h"

#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

TEST(TestMultihash, DeserializeSetsIdentifierAndDigest)
{
  std::vector<uint8_t> serialized = {0x12, 0x20};
  serialized.insert(serialized.end(), 32, 0xab);

  CMultihash multihash;

  EXPECT_TRUE(multihash.Deserialize(serialized));
  EXPECT_EQ(CMultihash::Identifier::SHA2_256, multihash.GetIdentifier());
  EXPECT_EQ(std::vector<uint8_t>(32, 0xab), multihash.GetData());
}

TEST(TestMultihash, DeserializeThenSerializeRoundTrips)
{
  std::vector<uint8_t> serialized = {0x12, 0x20};
  serialized.insert(serialized.end(), 32, 0xab);

  CMultihash multihash(serialized);
  std::vector<uint8_t> reserialized;

  EXPECT_TRUE(multihash.Serialize(reserialized));
  EXPECT_EQ(serialized, reserialized);
}

TEST(TestMultihash, FailedDeserializeLeavesStateUnchanged)
{
  CMultihash multihash(CMultihash::Identifier::SHA2_256, {0x01, 0x02, 0x03});

  EXPECT_FALSE(multihash.Deserialize({0x12, 0x20, 0xab}));
  EXPECT_EQ(CMultihash::Identifier::SHA2_256, multihash.GetIdentifier());
  EXPECT_EQ(std::vector<uint8_t>({0x01, 0x02, 0x03}), multihash.GetData());
}

TEST(TestMultihash, UnknownIdentifierFailsCleanly)
{
  CMultihash multihash(CMultihash::Identifier::SHA2_256, {0x01, 0x02, 0x03});

  EXPECT_FALSE(multihash.Deserialize({0xff, 0x00}));
  EXPECT_EQ(CMultihash::Identifier::SHA2_256, multihash.GetIdentifier());
  EXPECT_EQ(std::vector<uint8_t>({0x01, 0x02, 0x03}), multihash.GetData());
}

TEST(TestMultihash, DigestLengthLargerThanSingleByteIsRejected)
{
  CMultihash multihash(CMultihash::Identifier::SHA2_256, std::vector<uint8_t>(256, 0xab));
  std::vector<uint8_t> serialized = {0x12, 0x01, 0xab};

  EXPECT_FALSE(multihash.Serialize(serialized));
  EXPECT_EQ(std::vector<uint8_t>({0x12, 0x01, 0xab}), serialized);
}
