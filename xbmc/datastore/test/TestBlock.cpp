/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "datastore/Block.h"

#include <stdint.h>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;

TEST(TestBlock, DefaultConstructedBlockIsEmptyOwning)
{
  const CBlock block;

  EXPECT_TRUE(block.Empty());
  EXPECT_TRUE(block.IsOwning());
  EXPECT_EQ(block.Size(), 0U);
  EXPECT_TRUE(block.CID().Empty());
}

TEST(TestBlock, ConstructFromCIDAndDataOwnsBytes)
{
  const CCID cid(CIDCodec::RAW, {0x12, 0x20, 0xaa});
  const std::vector<uint8_t> data{1, 2, 3, 4};
  const CBlock block(cid, data);

  EXPECT_TRUE(block.IsOwning());
  EXPECT_EQ(block.CID(), cid);
  EXPECT_EQ(block.Size(), data.size());
  EXPECT_EQ(std::vector<uint8_t>(block.Data(), block.Data() + block.Size()), data);
}

TEST(TestBlock, FromBytesCopiesData)
{
  std::vector<uint8_t> data{1, 2, 3};
  const CCID cid(CIDCodec::RAW, {0x12});
  CBlock block = CBlock::FromBytes(cid, data.data(), data.size());

  data[0] = 9;

  EXPECT_TRUE(block.IsOwning());
  EXPECT_EQ(block.CID(), cid);
  EXPECT_EQ(block.Data()[0], 1);
}

TEST(TestBlock, ViewBytesReferencesData)
{
  std::vector<uint8_t> data{1, 2, 3};
  const CCID cid(CIDCodec::RAW, {0x12});
  CBlock block = CBlock::ViewBytes(cid, data.data(), data.size());

  EXPECT_FALSE(block.IsOwning());
  EXPECT_EQ(block.Data(), data.data());
  EXPECT_EQ(block.Size(), data.size());

  data[1] = 8;

  EXPECT_EQ(block.Serialize().first[1], 8);
}

TEST(TestBlock, CopyOwningBlockProducesIndependentOwningBlock)
{
  const CCID cid(CIDCodec::RAW, {0x12});
  CBlock original(cid, {1, 2, 3});
  CBlock copy(original);

  original.SetData(std::vector<uint8_t>{9, 9, 9});

  EXPECT_TRUE(copy.IsOwning());
  EXPECT_EQ(copy.CID(), cid);
  EXPECT_EQ(std::vector<uint8_t>(copy.Data(), copy.Data() + copy.Size()),
            (std::vector<uint8_t>{1, 2, 3}));
}

TEST(TestBlock, CopyViewBlockProducesIndependentOwningBlock)
{
  std::vector<uint8_t> data{1, 2, 3};
  const CCID cid(CIDCodec::RAW, {0x12});
  CBlock view = CBlock::ViewBytes(cid, data.data(), data.size());
  CBlock copy(view);

  data.assign(data.size(), 9);

  EXPECT_TRUE(copy.IsOwning());
  EXPECT_EQ(copy.CID(), cid);
  EXPECT_EQ(std::vector<uint8_t>(copy.Data(), copy.Data() + copy.Size()),
            (std::vector<uint8_t>{1, 2, 3}));
}

TEST(TestBlock, CopyAssignViewBlockProducesIndependentOwningBlock)
{
  std::vector<uint8_t> data{4, 5, 6};
  const CCID cid(CIDCodec::DAG_JSON, {0x12, 0x20});
  CBlock view = CBlock::ViewBytes(cid, data.data(), data.size());
  CBlock copy(CCID(CIDCodec::RAW, {0x12}), {1});

  copy = view;
  data.assign(data.size(), 9);

  EXPECT_TRUE(copy.IsOwning());
  EXPECT_EQ(copy.CID(), cid);
  EXPECT_EQ(std::vector<uint8_t>(copy.Data(), copy.Data() + copy.Size()),
            (std::vector<uint8_t>{4, 5, 6}));
}

TEST(TestBlock, CopySelfAssignmentKeepsBlockValid)
{
  const CCID cid(CIDCodec::RAW, {0x12});
  CBlock block(cid, {1, 2, 3});
  CBlock& alias = block;

  block = alias;

  EXPECT_TRUE(block.IsOwning());
  EXPECT_EQ(block.CID(), cid);
  EXPECT_EQ(std::vector<uint8_t>(block.Data(), block.Data() + block.Size()),
            (std::vector<uint8_t>{1, 2, 3}));
}

TEST(TestBlock, InvalidFactoryInputReturnsEmptyDefaultDataBlock)
{
  const CCID cid(CIDCodec::RAW, {0x12});
  CBlock block = CBlock::ViewBytes(cid, nullptr, 4);

  EXPECT_TRUE(block.IsOwning());
  EXPECT_TRUE(block.Empty());
  EXPECT_EQ(block.CID(), cid);

  block = CBlock::FromBytes(cid, nullptr, 4);
  EXPECT_TRUE(block.IsOwning());
  EXPECT_TRUE(block.Empty());
  EXPECT_EQ(block.CID(), cid);
}

TEST(TestBlock, InvalidMutatorInputPreservesPreviousData)
{
  const CCID cid(CIDCodec::RAW, {0x12});
  CBlock block(cid, {1, 2, 3});
  block.SetData(std::vector<uint8_t>{1, 2, 3});
  block.SetData(nullptr, 4);

  EXPECT_TRUE(block.IsOwning());
  EXPECT_EQ(std::vector<uint8_t>(block.Data(), block.Data() + block.Size()),
            (std::vector<uint8_t>{1, 2, 3}));

  block.SetDataView(nullptr, 4);

  EXPECT_TRUE(block.IsOwning());
  EXPECT_EQ(std::vector<uint8_t>(block.Data(), block.Data() + block.Size()),
            (std::vector<uint8_t>{1, 2, 3}));

  block.Deserialize(nullptr, 4);

  EXPECT_TRUE(block.IsOwning());
  EXPECT_EQ(std::vector<uint8_t>(block.Data(), block.Data() + block.Size()),
            (std::vector<uint8_t>{1, 2, 3}));
}

TEST(TestBlock, NullDataWithZeroSizeCanBeEmptyView)
{
  const CCID cid(CIDCodec::RAW, {0x12});
  CBlock block = CBlock::ViewBytes(cid, nullptr, 0);

  EXPECT_FALSE(block.IsOwning());
  EXPECT_TRUE(block.Empty());
  EXPECT_EQ(block.Data(), nullptr);
}

TEST(TestBlock, SetDataSwitchesViewToOwnedStorage)
{
  std::vector<uint8_t> viewed{1, 2, 3};
  CBlock block = CBlock::ViewBytes(CCID(CIDCodec::RAW, {0x12}), viewed.data(), viewed.size());

  block.SetData(std::vector<uint8_t>{4, 5});

  EXPECT_TRUE(block.IsOwning());
  EXPECT_EQ(block.Size(), 2U);
  EXPECT_EQ(std::vector<uint8_t>(block.Data(), block.Data() + block.Size()),
            (std::vector<uint8_t>{4, 5}));
}

TEST(TestBlock, DeserializeCopiesDataIntoOwnedStorage)
{
  std::vector<uint8_t> data{1, 2, 3};
  CBlock block = CBlock::ViewBytes(CCID(CIDCodec::RAW, {0x12}), data.data(), data.size());

  block.Deserialize(data.data(), data.size());
  data[0] = 9;

  EXPECT_TRUE(block.IsOwning());
  EXPECT_EQ(block.Data()[0], 1);
}

TEST(TestBlock, SetCIDReplacesCID)
{
  CBlock block(CCID(CIDCodec::RAW, {0x12}), {1});
  const CCID newCID(CIDCodec::DAG_JSON, {0x12, 0x20});

  block.SetCID(newCID);

  EXPECT_EQ(block.CID(), newCID);
}
