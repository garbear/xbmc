/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "datastore/Block.h"
#include "datastore/BlockStore.h"
#include "datastore/CID.h"
#include "datastore/DataStore.h"
#include "datastore/IDataStore.h"
#include "filesystem/Directory.h"
#include "utils/URIUtils.h"

#include <stdint.h>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;

namespace
{
class TestBlockStore : public testing::Test
{
protected:
  void SetUp() override
  {
    m_path = URIUtils::AddFileToFolder("special://temp", "kodi_blockstore_test");
    if (XFILE::CDirectory::Exists(m_path))
      XFILE::CDirectory::RemoveRecursive(m_path);
    ASSERT_TRUE(m_dataStore.Open(m_path));
  }

  void TearDown() override
  {
    m_dataStore.Close();
    XFILE::CDirectory::RemoveRecursive(m_path);
  }

  std::string m_path;
  CDataStore m_dataStore;
};
} // namespace

TEST_F(TestBlockStore, MissingCIDReturnsFalseAndLeavesOutputBlockUnchanged)
{
  CBlockStore blockStore(m_dataStore);
  const CCID missingCID(CIDCodec::RAW, {0x12, 0x20, 0xaa});
  const CBlock original(CCID(CIDCodec::RAW, {0x12}), {1, 2});
  CBlock output = original;

  EXPECT_FALSE(blockStore.Has(missingCID));
  EXPECT_FALSE(blockStore.Get(missingCID, output));
  EXPECT_EQ(output.CID(), original.CID());
  EXPECT_EQ(std::vector<uint8_t>(output.Data(), output.Data() + output.Size()),
            (std::vector<uint8_t>{1, 2}));
}

TEST_F(TestBlockStore, PutGetAndHasRoundTripBlock)
{
  CBlockStore blockStore(m_dataStore);
  const CCID cid(CIDCodec::RAW, {0x12, 0x20, 0xaa});
  const CBlock input(cid, {1, 2, 3, 4});

  ASSERT_TRUE(blockStore.Put(input));
  EXPECT_TRUE(blockStore.Has(cid));

  CBlock output;
  ASSERT_TRUE(blockStore.Get(cid, output));
  EXPECT_TRUE(output.IsOwning());
  EXPECT_EQ(output.CID(), cid);
  EXPECT_EQ(std::vector<uint8_t>(output.Data(), output.Data() + output.Size()),
            (std::vector<uint8_t>{1, 2, 3, 4}));
}

TEST_F(TestBlockStore, PutReplacesExistingBlockForCID)
{
  CBlockStore blockStore(m_dataStore);
  const CCID cid(CIDCodec::RAW, {0x12, 0x20, 0xaa});

  ASSERT_TRUE(blockStore.Put(CBlock(cid, {1})));
  ASSERT_TRUE(blockStore.Put(CBlock(cid, {2, 3})));

  CBlock output;
  ASSERT_TRUE(blockStore.Get(cid, output));
  EXPECT_EQ(std::vector<uint8_t>(output.Data(), output.Data() + output.Size()),
            (std::vector<uint8_t>{2, 3}));
}

TEST_F(TestBlockStore, DeleteRemovesStoredBlock)
{
  CBlockStore blockStore(m_dataStore);
  const CCID cid(CIDCodec::RAW, {0x12, 0x20, 0xaa});

  ASSERT_TRUE(blockStore.Put(CBlock(cid, {1, 2, 3})));
  ASSERT_TRUE(blockStore.Has(cid));

  EXPECT_TRUE(blockStore.Delete(cid));
  EXPECT_FALSE(blockStore.Has(cid));

  CBlock output;
  EXPECT_FALSE(blockStore.Get(cid, output));
}
