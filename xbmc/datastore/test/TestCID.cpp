/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/multiformats/Multicodec.h"
#include "crypto/multiformats/Multihash.h"
#include "crypto/multiformats/VarInt.h"
#include "datastore/CID.h"

#include <stdint.h>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;

namespace
{
std::vector<uint8_t> MakeSha2_256Multihash(std::vector<uint8_t> digest)
{
  CRYPTO::CMultihash multihash{CRYPTO::CMultihash::Identifier::SHA2_256, std::move(digest)};
  std::vector<uint8_t> serialized;
  EXPECT_TRUE(multihash.Serialize(serialized));
  return serialized;
}

std::vector<uint8_t> MakeCIDBytes(CIDCodec codec, const std::vector<uint8_t>& serializedMultihash)
{
  std::vector<uint8_t> bytes(CRYPTO::VarInt::EncodingLength(1));
  EXPECT_TRUE(CRYPTO::VarInt::EncodeTo(1, bytes.data(), bytes.size()));

  std::vector<uint8_t> encodedCodec;
  EXPECT_TRUE(CRYPTO::CMulticodec::Encode(codec, encodedCodec));
  bytes.insert(bytes.end(), encodedCodec.begin(), encodedCodec.end());
  bytes.insert(bytes.end(), serializedMultihash.begin(), serializedMultihash.end());
  return bytes;
}
} // namespace

TEST(TestCID, DefaultConstructedCIDIsEmptyOwningRaw)
{
  const CCID cid;

  EXPECT_TRUE(cid.Empty());
  EXPECT_TRUE(cid.IsOwning());
  EXPECT_EQ(cid.Size(), 0U);
  EXPECT_EQ(cid.Codec(), CIDCodec::RAW);
  EXPECT_TRUE(cid.Multihash().empty());
}

TEST(TestCID, ConstructFromCodecAndMultihashSerializesCIDV1)
{
  const std::vector<uint8_t> serialized = MakeSha2_256Multihash({0xaa, 0xbb});
  const CCID cid(CIDCodec::DAG_JSON, serialized);

  const std::vector<uint8_t> expected = MakeCIDBytes(CIDCodec::DAG_JSON, serialized);

  ASSERT_EQ(cid.Size(), expected.size());
  EXPECT_EQ(std::vector<uint8_t>(cid.Data(), cid.Data() + cid.Size()), expected);
  EXPECT_EQ(cid.Codec(), CIDCodec::DAG_JSON);
  EXPECT_EQ(cid.Multihash(), serialized);
}

TEST(TestCID, ConstructFromSerializedBytesParsesCodecAndMultihash)
{
  const std::vector<uint8_t> bytes{0x01, 0x55, 0x12, 0x20, 0x01, 0x02};
  const CCID cid(bytes);

  EXPECT_TRUE(cid.IsOwning());
  EXPECT_EQ(cid.Codec(), CIDCodec::RAW);
  EXPECT_EQ(cid.Multihash(), (std::vector<uint8_t>{0x12, 0x20, 0x01, 0x02}));
  EXPECT_EQ(std::vector<uint8_t>(cid.Data(), cid.Data() + cid.Size()), bytes);
}

TEST(TestCID, FromBytesCopiesSerializedBytes)
{
  std::vector<uint8_t> bytes{0x01, 0x55, 0x12, 0x20, 0xaa};
  CCID cid = CCID::FromBytes(bytes.data(), bytes.size());

  bytes[2] = 0xff;

  EXPECT_TRUE(cid.IsOwning());
  EXPECT_EQ(cid.Multihash(), (std::vector<uint8_t>{0x12, 0x20, 0xaa}));
  EXPECT_EQ(cid.Data()[2], 0x12);
}

TEST(TestCID, ViewBytesReferencesSerializedBytes)
{
  std::vector<uint8_t> bytes{0x01, 0xab, 0x02, 0x12, 0x20, 0xaa};
  CCID cid = CCID::ViewBytes(bytes.data(), bytes.size());

  EXPECT_FALSE(cid.IsOwning());
  EXPECT_EQ(cid.Data(), bytes.data());
  EXPECT_EQ(cid.Codec(), CIDCodec::DAG_JSON_ZSTD);
  EXPECT_EQ(cid.Multihash(), (std::vector<uint8_t>{0x12, 0x20, 0xaa}));

  bytes[5] = 0xbb;

  EXPECT_EQ(cid.Serialize().first[5], 0xbb);
}

TEST(TestCID, CopyOwningCIDProducesIndependentOwningCID)
{
  std::vector<uint8_t> bytes{0x01, 0x55, 0x12, 0x20, 0xaa};
  CCID original(bytes);
  CCID copy(original);

  bytes[4] = 0xbb;
  original.Deserialize(bytes);

  EXPECT_TRUE(copy.IsOwning());
  EXPECT_EQ(copy.Codec(), CIDCodec::RAW);
  EXPECT_EQ(copy.Multihash(), (std::vector<uint8_t>{0x12, 0x20, 0xaa}));
  EXPECT_EQ(std::vector<uint8_t>(copy.Data(), copy.Data() + copy.Size()),
            (std::vector<uint8_t>{0x01, 0x55, 0x12, 0x20, 0xaa}));
}

TEST(TestCID, CopyViewCIDProducesIndependentOwningCID)
{
  std::vector<uint8_t> bytes{0x01, 0xab, 0x02, 0x12, 0x20, 0xaa};
  CCID view = CCID::ViewBytes(bytes.data(), bytes.size());
  CCID copy(view);

  bytes.assign(bytes.size(), 0xff);

  EXPECT_TRUE(copy.IsOwning());
  EXPECT_EQ(copy.Codec(), CIDCodec::DAG_JSON_ZSTD);
  EXPECT_EQ(copy.Multihash(), (std::vector<uint8_t>{0x12, 0x20, 0xaa}));
  EXPECT_EQ(std::vector<uint8_t>(copy.Data(), copy.Data() + copy.Size()),
            (std::vector<uint8_t>{0x01, 0xab, 0x02, 0x12, 0x20, 0xaa}));
}

TEST(TestCID, CopyAssignViewCIDProducesIndependentOwningCID)
{
  std::vector<uint8_t> bytes{0x01, 0xa9, 0x02, 0x12, 0x20, 0xaa};
  CCID view = CCID::ViewBytes(bytes.data(), bytes.size());
  CCID copy(CIDCodec::RAW, {0x12});

  copy = view;
  bytes.assign(bytes.size(), 0xff);

  EXPECT_TRUE(copy.IsOwning());
  EXPECT_EQ(copy.Codec(), CIDCodec::DAG_JSON);
  EXPECT_EQ(copy.Multihash(), (std::vector<uint8_t>{0x12, 0x20, 0xaa}));
  EXPECT_EQ(std::vector<uint8_t>(copy.Data(), copy.Data() + copy.Size()),
            (std::vector<uint8_t>{0x01, 0xa9, 0x02, 0x12, 0x20, 0xaa}));
}

TEST(TestCID, CopySelfAssignmentKeepsCIDValid)
{
  CCID cid(CIDCodec::DAG_JSON_LZ4, {0x12, 0x20, 0xaa});
  CCID& alias = cid;

  cid = alias;

  EXPECT_TRUE(cid.IsOwning());
  EXPECT_EQ(cid.Codec(), CIDCodec::DAG_JSON_LZ4);
  EXPECT_EQ(cid.Multihash(), (std::vector<uint8_t>{0x12, 0x20, 0xaa}));
}

TEST(TestCID, InvalidFactoryInputReturnsEmptyDefaultCID)
{
  CCID cid = CCID::ViewBytes(nullptr, 4);

  EXPECT_TRUE(cid.Empty());
  EXPECT_TRUE(cid.IsOwning());
  EXPECT_EQ(cid.Codec(), CIDCodec::RAW);
  EXPECT_TRUE(cid.Multihash().empty());

  cid = CCID::FromBytes(nullptr, 4);
  EXPECT_TRUE(cid.Empty());
  EXPECT_TRUE(cid.IsOwning());
  EXPECT_EQ(cid.Codec(), CIDCodec::RAW);
  EXPECT_TRUE(cid.Multihash().empty());
}

TEST(TestCID, InvalidMutatorInputPreservesPreviousState)
{
  CCID cid;
  const std::vector<uint8_t> originalBytes{0x01, 0x55, 0x12, 0x20, 0xaa};
  cid.Deserialize(originalBytes);

  cid.Deserialize(nullptr, 4);
  EXPECT_EQ(std::vector<uint8_t>(cid.Data(), cid.Data() + cid.Size()), originalBytes);
  EXPECT_TRUE(cid.IsOwning());
  EXPECT_EQ(cid.Codec(), CIDCodec::RAW);
  EXPECT_EQ(cid.Multihash(), (std::vector<uint8_t>{0x12, 0x20, 0xaa}));
}

TEST(TestCID, MutatorsRegenerateOwnedSerializedBytes)
{
  std::vector<uint8_t> bytes{0x01, 0x55, 0x12};
  CCID cid = CCID::ViewBytes(bytes.data(), bytes.size());

  cid.SetCodec(CIDCodec::DAG_JSON_LZ4);
  cid.SetMultihash({0x12, 0x20, 0xcc});

  const std::vector<uint8_t> expected{0x01, 0xaa, 0x02, 0x12, 0x20, 0xcc};

  EXPECT_TRUE(cid.IsOwning());
  EXPECT_EQ(cid.Codec(), CIDCodec::DAG_JSON_LZ4);
  EXPECT_EQ(cid.Multihash(), (std::vector<uint8_t>{0x12, 0x20, 0xcc}));
  EXPECT_EQ(std::vector<uint8_t>(cid.Data(), cid.Data() + cid.Size()), expected);
}

TEST(TestCID, EqualityComparesSerializedBytes)
{
  const CCID first(CIDCodec::RAW, {0x12, 0x20, 0xaa});
  const CCID second(CIDCodec::RAW, {0x12, 0x20, 0xaa});
  const CCID third(CIDCodec::RAW, {0x12, 0x20, 0xbb});

  EXPECT_TRUE(first == second);
  EXPECT_FALSE(first == third);
}

TEST(TestCID, CIDStringRoundTrips)
{
  const std::vector<uint8_t> serialized = MakeSha2_256Multihash({0xaa, 0xbb, 0xcc});
  const CCID original(CIDCodec::RAW, serialized);

  const std::string cidString = original.ToString();
  ASSERT_FALSE(cidString.empty());
  EXPECT_EQ(cidString.front(), 'z');

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cidString, parsed));

  EXPECT_EQ(std::vector<uint8_t>(parsed.Data(), parsed.Data() + parsed.Size()),
            std::vector<uint8_t>(original.Data(), original.Data() + original.Size()));
  EXPECT_EQ(parsed.Codec(), CIDCodec::RAW);
  EXPECT_EQ(parsed.Multihash(), serialized);
}

TEST(TestCID, DAGJsonZstdCIDStringRoundTrips)
{
  const std::vector<uint8_t> serialized = MakeSha2_256Multihash({0xaa, 0xbb, 0xcc});
  const CCID original(CIDCodec::DAG_JSON_ZSTD, serialized);
  const std::vector<uint8_t> expected = MakeCIDBytes(CIDCodec::DAG_JSON_ZSTD, serialized);

  ASSERT_EQ(original.Size(), expected.size());
  EXPECT_EQ(std::vector<uint8_t>(original.Data(), original.Data() + original.Size()), expected);

  const std::string cidString = original.ToString();
  ASSERT_FALSE(cidString.empty());

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cidString, parsed));
  EXPECT_EQ(std::vector<uint8_t>(parsed.Data(), parsed.Data() + parsed.Size()), expected);
  EXPECT_EQ(parsed.Codec(), CIDCodec::DAG_JSON_ZSTD);
  EXPECT_EQ(parsed.Multihash(), serialized);
}

TEST(TestCID, InvalidCIDStringLeavesOutputUnchanged)
{
  const CCID original(CIDCodec::RAW, {0x12, 0x20, 0xaa});
  CCID output = original;

  EXPECT_FALSE(CCID::FromString("not-a-cid", output));
  EXPECT_EQ(output, original);
}

TEST(TestCID, InvalidSerializedBytesKeepBytesButClearParsedFields)
{
  const std::vector<uint8_t> bytes{0x02, 0x55, 0x12, 0x20};
  const CCID cid(bytes);

  EXPECT_FALSE(cid.Empty());
  EXPECT_EQ(cid.Codec(), CIDCodec::RAW);
  EXPECT_TRUE(cid.Multihash().empty());
  EXPECT_EQ(std::vector<uint8_t>(cid.Data(), cid.Data() + cid.Size()), bytes);
}
