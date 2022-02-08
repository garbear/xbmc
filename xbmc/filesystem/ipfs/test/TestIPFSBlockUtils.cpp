/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/multiformats/Multihash.h"
#include "datastore/Block.h"
#include "datastore/CID.h"
#include "filesystem/ipfs/block/IPFSBlockUtils.h"
#include "filesystem/ipfs/test/IPFSTestUtils.h"

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;
using namespace XFILE;
using namespace XFILE::IPFS::TEST;

TEST(TestIPFSBlockUtils, MakeAddressedBlockRejectsNullNonzero)
{
  CCID cid;
  CBlock block;

  EXPECT_FALSE(IPFS::CIPFSBlockUtils::MakeAddressedBlock(CIDCodec::RAW, nullptr, 1, cid, block));
  EXPECT_TRUE(cid.Empty());
  EXPECT_TRUE(block.Empty());
}

TEST(TestIPFSBlockUtils, MakeAddressedBlockAllowsNullZero)
{
  CCID cid;
  CBlock block;

  ASSERT_TRUE(IPFS::CIPFSBlockUtils::MakeAddressedBlock(CIDCodec::RAW, nullptr, 0, cid, block));
  EXPECT_FALSE(cid.Empty());
  EXPECT_TRUE(block.Empty());
  EXPECT_EQ(cid.Codec(), CIDCodec::RAW);
  EXPECT_EQ(cid.Multihash(), MakeSha2_256Multihash({}));
}

TEST(TestIPFSBlockUtils, MakeAddressedBlockUsesRequestedCodec)
{
  const std::vector<uint8_t> payload{'a', 'b', 'c'};
  CCID cid;
  CBlock block;

  ASSERT_TRUE(IPFS::CIPFSBlockUtils::MakeAddressedBlock(CIDCodec::DAG_JSON, payload.data(),
                                                        payload.size(), cid, block));
  EXPECT_EQ(cid.Codec(), CIDCodec::DAG_JSON);

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid.ToString(), parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::DAG_JSON);
  EXPECT_EQ(parsed.Multihash(), cid.Multihash());
}

TEST(TestIPFSBlockUtils, MakeAddressedBlockHashesExactPayloadBytes)
{
  const std::vector<uint8_t> payload{0, 1, 2, 3, 255};
  CCID cid;
  CBlock block;

  ASSERT_TRUE(IPFS::CIPFSBlockUtils::MakeAddressedBlock(CIDCodec::RAW, payload.data(),
                                                        payload.size(), cid, block));
  EXPECT_EQ(cid.Multihash(), MakeSha2_256Multihash(payload));
  ASSERT_EQ(block.Size(), payload.size());
  ASSERT_NE(block.Data(), nullptr);
  EXPECT_TRUE(std::equal(payload.begin(), payload.end(), block.Data()));
}

TEST(TestIPFSBlockUtils, DifferentPayloadsProduceDifferentCIDs)
{
  const std::vector<uint8_t> first{'a'};
  const std::vector<uint8_t> second{'b'};
  CCID firstCid;
  CBlock firstBlock;
  CCID secondCid;
  CBlock secondBlock;

  ASSERT_TRUE(IPFS::CIPFSBlockUtils::MakeAddressedBlock(CIDCodec::RAW, first.data(), first.size(),
                                                        firstCid, firstBlock));
  ASSERT_TRUE(IPFS::CIPFSBlockUtils::MakeAddressedBlock(CIDCodec::RAW, second.data(), second.size(),
                                                        secondCid, secondBlock));
  EXPECT_NE(firstCid.ToString(), secondCid.ToString());
  EXPECT_NE(firstCid.Multihash(), secondCid.Multihash());
}

TEST(TestIPFSBlockUtils, MakeAddressedBlockUsesCryptoMultihash)
{
  const std::vector<uint8_t> payload{'a', 'b', 'c'};
  CCID cid;
  CBlock block;

  ASSERT_TRUE(IPFS::CIPFSBlockUtils::MakeAddressedBlock(CIDCodec::RAW, payload.data(),
                                                        payload.size(), cid, block));

  EXPECT_EQ(cid.Multihash(), MakeSha2_256Multihash(payload));
}

TEST(TestIPFSBlockUtils, EmptyPayloadCIDUsesFullSha2256Multihash)
{
  CCID cid;
  CBlock block;

  ASSERT_TRUE(IPFS::CIPFSBlockUtils::MakeAddressedBlock(CIDCodec::RAW, nullptr, 0, cid, block));
  ASSERT_EQ(cid.Multihash().size(), 34U);
  EXPECT_EQ(cid.Multihash()[0], 0x12);
  EXPECT_EQ(cid.Multihash()[1], 0x20);

  CRYPTO::CMultihash parsed(cid.Multihash());
  EXPECT_EQ(parsed.GetIdentifier(), CRYPTO::CMultihash::Identifier::SHA2_256);
  EXPECT_EQ(parsed.GetData().size(), 32U);
}
