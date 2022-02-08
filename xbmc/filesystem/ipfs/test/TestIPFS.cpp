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
#include "filesystem/ipfs/IPFS.h"
#include "filesystem/ipfs/IPFSService.h"
#include "utils/Variant.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;
using namespace XFILE;

namespace
{
std::filesystem::path TempIPFSPath(const std::string& name)
{
  const auto path = std::filesystem::temp_directory_path() / ("kodi_ipfs_test_" + name);
  std::filesystem::remove_all(path);
  return path;
}
} // namespace

TEST(TestIPFS, AddTextFileAndReadBack)
{
  const auto path = TempIPFSPath("txt_round_trip");
  const std::string input = "hello from kodi ipfs\n";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));
  ASSERT_FALSE(cid.empty());

  std::vector<uint8_t> output;
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(input, std::string(output.begin(), output.end()));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::RAW);
  EXPECT_FALSE(parsed.Multihash().empty());

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open((path / "ipfs").string()));
  CBlockStore blockStore(dataStore);
  EXPECT_TRUE(blockStore.Has(parsed));
  dataStore.Close();

  ASSERT_TRUE(ipfs.Initialize(path.string()));
  output.clear();
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(input, std::string(output.begin(), output.end()));

  ipfs.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, InitializeRejectsEmptyPathAndTracksOnlineState)
{
  CIPFS ipfs;
  EXPECT_FALSE(ipfs.IsOnline());
  EXPECT_FALSE(ipfs.Initialize(""));
  EXPECT_FALSE(ipfs.IsOnline());

  const auto path = TempIPFSPath("online_state");
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  ipfs.Deinitialize();
  EXPECT_FALSE(ipfs.IsOnline());
  std::filesystem::remove_all(path);
}

TEST(TestIPFSService, InitializeCreatesProfileDatastoreDirectory)
{
  const auto path = TempIPFSPath("service_datastore_directory");
  const std::string input = "service bytes\n";

  CIPFSService service;
  ASSERT_TRUE(service.Initialize(path.string()));
  EXPECT_TRUE(std::filesystem::is_directory(path / "ipfs"));

  std::string cid;
  ASSERT_TRUE(service.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));
  ASSERT_FALSE(cid.empty());

  std::vector<uint8_t> output;
  ASSERT_TRUE(service.GetFile(cid, output));
  EXPECT_EQ(input, std::string(output.begin(), output.end()));

  service.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFSService, InitializeIsIdempotentForSamePath)
{
  const auto path = TempIPFSPath("service_idempotent_same_path");

  CIPFSService service;
  ASSERT_TRUE(service.Initialize(path.string()));
  EXPECT_TRUE(service.Initialize(path.string()));

  service.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, EmptyFileRoundTrips)
{
  const auto path = TempIPFSPath("empty_file");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(nullptr, 0, cid));
  ASSERT_FALSE(cid.empty());

  std::vector<uint8_t> output{1, 2, 3};
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_TRUE(output.empty());

  ipfs.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, SameBytesProduceSameCID)
{
  const auto path = TempIPFSPath("same_bytes");
  const std::string input = "same bytes\n";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  std::string first;
  std::string second;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), first));
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), second));

  EXPECT_EQ(first, second);
  ipfs.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, DifferentBytesProduceDifferentCID)
{
  const auto path = TempIPFSPath("different_bytes");
  const std::string firstInput = "first\n";
  const std::string secondInput = "second\n";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  std::string first;
  std::string second;
  ASSERT_TRUE(
      ipfs.AddFile(reinterpret_cast<const uint8_t*>(firstInput.data()), firstInput.size(), first));
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(secondInput.data()), secondInput.size(),
                           second));

  EXPECT_NE(first, second);
  ipfs.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, AddFileReturnsCIDForStoredRawBlock)
{
  const auto path = TempIPFSPath("stored_raw_block");
  const std::string input = "stored raw block\n";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::RAW);

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open((path / "ipfs").string()));
  CBlockStore blockStore(dataStore);

  CBlock block;
  ASSERT_TRUE(blockStore.Get(parsed, block));
  EXPECT_EQ(input, std::string(block.Data(), block.Data() + block.Size()));

  dataStore.Close();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, GetFileDoesNotRequireRehash)
{
  const auto path = TempIPFSPath("get_file_no_rehash");
  const std::string input = "addressed bytes\n";
  const std::vector<uint8_t> replacement{'r', 'e', 'p', 'l', 'a', 'c', 'e', 'd'};

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open((path / "ipfs").string()));
  CBlockStore blockStore(dataStore);
  ASSERT_TRUE(blockStore.Put(CBlock(parsed, replacement)));
  dataStore.Close();

  ASSERT_TRUE(ipfs.Initialize(path.string()));

  std::vector<uint8_t> output;
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(output, replacement);

  ipfs.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, AddFileRejectsNullNonzero)
{
  const auto path = TempIPFSPath("null_nonzero");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  std::string cid = "unchanged";
  EXPECT_FALSE(ipfs.AddFile(nullptr, 1, cid));
  EXPECT_EQ(cid, "unchanged");

  ipfs.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, FileOperationsFailWhenOffline)
{
  CIPFS ipfs;

  const std::string input = "offline\n";
  std::string cid = "unchanged";
  EXPECT_FALSE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));
  EXPECT_EQ(cid, "unchanged");

  std::vector<uint8_t> output{1, 2, 3};
  EXPECT_FALSE(ipfs.GetFile("not-a-cid", output));
  EXPECT_EQ(output, (std::vector<uint8_t>{1, 2, 3}));
}

TEST(TestIPFS, GetFileRejectsInvalidCID)
{
  const auto path = TempIPFSPath("invalid_cid");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  std::vector<uint8_t> output{1, 2, 3};
  EXPECT_FALSE(ipfs.GetFile("not-a-cid", output));
  EXPECT_EQ(output, (std::vector<uint8_t>{1, 2, 3}));

  ipfs.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, PutDAGAndGetDAGRoundTrip)
{
  const auto path = TempIPFSPath("dag_round_trip");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path.string()));

  CVariant content(CVariant::VariantTypeObject);
  content["name"] = "Kodi";
  content["answer"] = 42;
  content["enabled"] = true;

  const std::string cid = ipfs.PutDAG(content);
  ASSERT_FALSE(cid.empty());

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::DAG_JSON);

  const CVariant output = ipfs.GetDAG(cid);
  EXPECT_TRUE(output.isObject());
  EXPECT_EQ(output["name"].asString(), "Kodi");
  EXPECT_EQ(output["answer"].asInteger(), 42);
  EXPECT_TRUE(output["enabled"].asBoolean());

  ipfs.Deinitialize();
  std::filesystem::remove_all(path);
}

TEST(TestIPFS, DAGOperationsRejectOfflineAndInvalidCID)
{
  CIPFS ipfs;
  CVariant content(CVariant::VariantTypeObject);
  content["name"] = "Kodi";

  EXPECT_TRUE(ipfs.PutDAG(content).empty());
  EXPECT_TRUE(ipfs.GetDAG("not-a-cid").isNull());

  const auto path = TempIPFSPath("dag_invalid");
  ASSERT_TRUE(ipfs.Initialize(path.string()));
  EXPECT_TRUE(ipfs.GetDAG("not-a-cid").isNull());

  ipfs.Deinitialize();
  std::filesystem::remove_all(path);
}
