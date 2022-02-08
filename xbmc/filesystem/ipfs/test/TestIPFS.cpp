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
#include "filesystem/ipfs/IPFS.h"
#include "filesystem/ipfs/IPFSService.h"
#include "filesystem/ipfs/ZstdCompression.h"
#include "filesystem/ipfs/test/IPFSTestUtils.h"
#include "filesystem/ipfs/unixfs/UnixFSCodec.h"
#include "filesystem/ipfs/unixfs/UnixFSJson.h"
#include "utils/JSONVariantWriter.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;
using namespace XFILE;
using namespace XFILE::IPFS::TEST;

namespace
{
constexpr size_t UNIXFS_JSON_TEST_INLINE_MAX_SIZE = 256 * 1024;
constexpr size_t UNIXFS_JSON_TEST_CHUNK_SIZE = 256 * 1024;

std::string TempIPFSPath(const std::string& name)
{
  const std::string path = URIUtils::AddFileToFolder("special://temp", "kodi_ipfs_test_" + name);
  if (XFILE::CDirectory::Exists(path))
    XFILE::CDirectory::RemoveRecursive(path);
  return path;
}

bool StoreTestBlock(CBlockStore& blockStore,
                    CIDCodec codec,
                    const std::vector<uint8_t>& data,
                    CCID& cid)
{
  cid = MakeTestCID(codec, data);
  return blockStore.Put(CBlock(cid, data));
}

bool DecodeStoredUnixFSRoot(CBlockStore& blockStore, const CCID& cid, IPFS::UnixFSJsonNode& node)
{
  CBlock block;
  if (!blockStore.Get(cid, block))
    return false;

  std::vector<uint8_t> payload;
  if (!IPFS::UNIXFS::CCodec::DecodeRootPayload(cid.Codec(), block.Data(), block.Size(), payload))
    return false;

  return IPFS::CUnixFSJson::DecodeNode(payload.data(), payload.size(), node);
}
} // namespace

TEST(TestIPFS, AddTextFileAndReadBack)
{
  const auto path = TempIPFSPath("txt_round_trip");
  const std::string input = "hello from kodi ipfs\n";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));
  ASSERT_FALSE(cid.empty());

  std::vector<uint8_t> output;
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(input, std::string(output.begin(), output.end()));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::DAG_JSON);
  EXPECT_FALSE(parsed.Multihash().empty());

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);
  EXPECT_TRUE(blockStore.Has(parsed));
  dataStore.Close();

  ASSERT_TRUE(ipfs.Initialize(path));
  output.clear();
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(input, std::string(output.begin(), output.end()));

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, AddFileReturnsUnixFSJsonCID)
{
  const auto path = TempIPFSPath("unixfs_json_root");
  const std::string input = "test";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::DAG_JSON);

  std::vector<uint8_t> output;
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(input, std::string(output.begin(), output.end()));

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, AddEmptyFileReturnsUnixFSJsonCID)
{
  const auto path = TempIPFSPath("empty_unixfs_json_root");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(nullptr, 0, cid));
  ASSERT_FALSE(cid.empty());

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::DAG_JSON);

  std::vector<uint8_t> output{1, 2, 3};
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_TRUE(output.empty());

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, InitializeRejectsEmptyPathAndTracksOnlineState)
{
  CIPFS ipfs;
  EXPECT_FALSE(ipfs.IsOnline());
  EXPECT_FALSE(ipfs.Initialize(""));
  EXPECT_FALSE(ipfs.IsOnline());

  const auto path = TempIPFSPath("online_state");
  ASSERT_TRUE(ipfs.Initialize(path));

  ipfs.Deinitialize();
  EXPECT_FALSE(ipfs.IsOnline());
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFSService, InitializeCreatesProfileDatastoreDirectory)
{
  const auto path = TempIPFSPath("service_datastore_directory");
  const std::string input = "service bytes\n";

  CIPFSService service;
  ASSERT_TRUE(service.Initialize(path));
  EXPECT_TRUE(XFILE::CDirectory::Exists(URIUtils::AddFileToFolder(path, "ipfs")));

  std::string cid;
  ASSERT_TRUE(service.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));
  ASSERT_FALSE(cid.empty());

  std::vector<uint8_t> output;
  ASSERT_TRUE(service.GetFile(cid, output));
  EXPECT_EQ(input, std::string(output.begin(), output.end()));

  service.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFSService, InitializeIsIdempotentForSamePath)
{
  const auto path = TempIPFSPath("service_idempotent_same_path");

  CIPFSService service;
  ASSERT_TRUE(service.Initialize(path));
  EXPECT_TRUE(service.Initialize(path));

  service.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFSService, RepeatedInitializeAndDeinitializeIsSafe)
{
  const auto path = TempIPFSPath("service_repeated_lifecycle");

  CIPFSService service;
  EXPECT_FALSE(service.IsInitialized());
  ASSERT_TRUE(service.Initialize(path));
  EXPECT_TRUE(service.IsInitialized());

  service.Deinitialize();
  EXPECT_FALSE(service.IsInitialized());
  service.Deinitialize();
  EXPECT_FALSE(service.IsInitialized());

  ASSERT_TRUE(service.Initialize(path));
  EXPECT_TRUE(service.IsInitialized());
  service.Deinitialize();
  EXPECT_FALSE(service.IsInitialized());

  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFSService, OperationsFailAfterDeinitializeWithoutOverwritingOutputs)
{
  const auto path = TempIPFSPath("service_offline_after_deinit");

  CIPFSService service;
  ASSERT_TRUE(service.Initialize(path));
  service.Deinitialize();
  EXPECT_FALSE(service.IsInitialized());

  std::string cid = "unchanged";
  const uint8_t data[] = {'t', 'e', 's', 't'};
  EXPECT_FALSE(service.AddFile(data, sizeof(data), cid));
  EXPECT_EQ("unchanged", cid);

  std::vector<uint8_t> output = {0x01, 0x02, 0x03};
  EXPECT_FALSE(service.GetFile("bafy", output));
  EXPECT_EQ(output, std::vector<uint8_t>({0x01, 0x02, 0x03}));

  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, EmptyFileRoundTrips)
{
  const auto path = TempIPFSPath("empty_file");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(nullptr, 0, cid));
  ASSERT_FALSE(cid.empty());

  std::vector<uint8_t> output{1, 2, 3};
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_TRUE(output.empty());

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, SameBytesProduceSameCID)
{
  const auto path = TempIPFSPath("same_bytes");
  const std::string input = "same bytes\n";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string first;
  std::string second;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), first));
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), second));

  EXPECT_EQ(first, second);
  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, DifferentBytesProduceDifferentCID)
{
  const auto path = TempIPFSPath("different_bytes");
  const std::string firstInput = "first\n";
  const std::string secondInput = "second\n";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string first;
  std::string second;
  ASSERT_TRUE(
      ipfs.AddFile(reinterpret_cast<const uint8_t*>(firstInput.data()), firstInput.size(), first));
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(secondInput.data()), secondInput.size(),
                           second));

  EXPECT_NE(first, second);
  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, AddFileReturnsCIDForStoredUnixFSJsonBlock)
{
  const auto path = TempIPFSPath("stored_unixfs_json_block");
  const std::string input = "stored unixfs json block\n";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::DAG_JSON);

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  CBlock block;
  ASSERT_TRUE(blockStore.Get(parsed, block));

  IPFS::UnixFSJsonNode node;
  ASSERT_TRUE(IPFS::CUnixFSJson::DecodeNode(block.Data(), block.Size(), node));
  EXPECT_EQ(input, std::string(node.data.begin(), node.data.end()));

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, AddSmallFileStoresInlineDAGJsonRoot)
{
  const auto path = TempIPFSPath("small_file_inline_json_root");
  const std::string input = "small inline file\n";

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  ASSERT_EQ(parsed.Codec(), CIDCodec::DAG_JSON);

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  IPFS::UnixFSJsonNode node;
  ASSERT_TRUE(DecodeStoredUnixFSRoot(blockStore, parsed, node));
  EXPECT_EQ(node.type, IPFS::UnixFSJsonNodeType::File);
  EXPECT_EQ(input, std::string(node.data.begin(), node.data.end()));
  EXPECT_TRUE(node.links.empty());

  dataStore.Close();

  ASSERT_TRUE(ipfs.Initialize(path));
  std::vector<uint8_t> output;
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(input, std::string(output.begin(), output.end()));

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, AddLargeFileStoresDAGJsonRootWithRawLeaves)
{
  const auto path = TempIPFSPath("large_file_raw_leaves");
  std::vector<uint8_t> input(UNIXFS_JSON_TEST_INLINE_MAX_SIZE + UNIXFS_JSON_TEST_CHUNK_SIZE + 123);
  for (size_t i = 0; i < input.size(); ++i)
    input[i] = static_cast<uint8_t>(i % 251);

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(input.data(), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  ASSERT_TRUE(parsed.Codec() == CIDCodec::DAG_JSON || parsed.Codec() == CIDCodec::DAG_JSON_ZSTD);

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  IPFS::UnixFSJsonNode node;
  ASSERT_TRUE(DecodeStoredUnixFSRoot(blockStore, parsed, node));
  EXPECT_EQ(node.type, IPFS::UnixFSJsonNodeType::File);
  EXPECT_TRUE(node.data.empty());
  EXPECT_EQ(node.fileSize, input.size());

  const size_t expectedChunks =
      (input.size() + UNIXFS_JSON_TEST_CHUNK_SIZE - 1) / UNIXFS_JSON_TEST_CHUNK_SIZE;
  ASSERT_EQ(node.links.size(), expectedChunks);

  size_t offset = 0;
  for (const IPFS::UnixFSJsonLink& link : node.links)
  {
    CCID childCid;
    ASSERT_TRUE(CCID::FromString(link.cid, childCid));
    EXPECT_EQ(childCid.Codec(), CIDCodec::RAW);

    CBlock childBlock;
    ASSERT_TRUE(blockStore.Get(childCid, childBlock));
    ASSERT_EQ(childBlock.Size(), link.blockSize);
    ASSERT_EQ(childBlock.Size(), std::min(UNIXFS_JSON_TEST_CHUNK_SIZE, input.size() - offset));
    EXPECT_TRUE(std::equal(input.begin() + offset, input.begin() + offset + childBlock.Size(),
                           childBlock.Data()));
    offset += childBlock.Size();
  }
  EXPECT_EQ(offset, input.size());

  dataStore.Close();

  ASSERT_TRUE(ipfs.Initialize(path));
  std::vector<uint8_t> output;
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(output, input);

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, ChunkedFileRootCIDHashesStoredRootBytes)
{
  const auto path = TempIPFSPath("chunked_root_hashes_stored_bytes");
  const std::vector<uint8_t> input(UNIXFS_JSON_TEST_INLINE_MAX_SIZE + 1, 'A');

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(input.data(), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  CBlock rootBlock;
  ASSERT_TRUE(blockStore.Get(parsed, rootBlock));
  ASSERT_FALSE(rootBlock.Empty());
  EXPECT_EQ(parsed.Multihash(), MakeSha2_256Multihash(rootBlock.Data(), rootBlock.Size()));

  if (parsed.Codec() == CIDCodec::DAG_JSON_ZSTD)
  {
    std::vector<uint8_t> decompressed;
    ASSERT_TRUE(IPFS::DecompressZstd(rootBlock.Data(), rootBlock.Size(), decompressed));
    EXPECT_NE(parsed.Multihash(), MakeSha2_256Multihash(decompressed.data(), decompressed.size()));
  }

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, AddFileCanReturnUnixFSJsonZstdCID)
{
  const auto path = TempIPFSPath("unixfs_json_zstd_root");
  const std::vector<uint8_t> input(32 * 1024, 'A');

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(input.data(), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  ASSERT_EQ(parsed.Codec(), CIDCodec::DAG_JSON_ZSTD);

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  CBlock block;
  ASSERT_TRUE(blockStore.Get(parsed, block));
  ASSERT_FALSE(block.Empty());
  EXPECT_EQ(parsed.Multihash(), MakeSha2_256Multihash(block.Data(), block.Size()));

  dataStore.Close();

  ASSERT_TRUE(ipfs.Initialize(path));
  std::vector<uint8_t> output;
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(input, output);

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, GetFileRejectsMissingChunk)
{
  const auto path = TempIPFSPath("missing_chunk");
  const std::vector<uint8_t> missingChunk{'m', 'i', 's', 's', 'i', 'n', 'g'};
  const CCID missingCid = MakeTestCID(CIDCodec::RAW, missingChunk);

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::File;
  node.fileSize = missingChunk.size();
  node.links = {{missingCid.ToString(), "", missingChunk.size(), missingChunk.size()}};

  std::vector<uint8_t> rootBytes;
  ASSERT_TRUE(IPFS::CUnixFSJson::EncodeNode(node, rootBytes));

  const auto rootCid = MakeTestCID(CIDCodec::DAG_JSON, rootBytes);

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);
  ASSERT_TRUE(blockStore.Put(CBlock(rootCid, rootBytes)));
  dataStore.Close();

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::vector<uint8_t> output{1, 2, 3};
  EXPECT_FALSE(ipfs.GetFile(rootCid.ToString(), output));
  EXPECT_EQ(output, (std::vector<uint8_t>{1, 2, 3}));

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, GetFileRejectsWrongChunkSize)
{
  const auto path = TempIPFSPath("wrong_chunk_size");
  const std::vector<uint8_t> chunk{'c', 'h', 'u', 'n', 'k'};

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  CCID childCid;
  ASSERT_TRUE(StoreTestBlock(blockStore, CIDCodec::RAW, chunk, childCid));

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::File;
  node.fileSize = chunk.size() + 1;
  node.links = {{childCid.ToString(), "", chunk.size() + 1, chunk.size() + 1}};

  std::vector<uint8_t> rootBytes;
  ASSERT_TRUE(IPFS::CUnixFSJson::EncodeNode(node, rootBytes));

  CCID rootCid;
  ASSERT_TRUE(StoreTestBlock(blockStore, CIDCodec::DAG_JSON, rootBytes, rootCid));
  dataStore.Close();

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::vector<uint8_t> output{1, 2, 3};
  EXPECT_FALSE(ipfs.GetFile(rootCid.ToString(), output));
  EXPECT_EQ(output, (std::vector<uint8_t>{1, 2, 3}));

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, GetFileRejectsRawRoot)
{
  const auto path = TempIPFSPath("raw_root_rejected");
  const std::vector<uint8_t> raw{'r', 'a', 'w'};

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  CCID rawCid;
  ASSERT_TRUE(StoreTestBlock(blockStore, CIDCodec::RAW, raw, rawCid));
  dataStore.Close();

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::vector<uint8_t> output{1, 2, 3};
  EXPECT_FALSE(ipfs.GetFile(rawCid.ToString(), output));
  EXPECT_EQ(output, (std::vector<uint8_t>{1, 2, 3}));

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, GetFileDoesNotRequireRehash)
{
  const auto path = TempIPFSPath("get_file_no_rehash");
  const std::string input = "addressed bytes\n";
  const std::vector<uint8_t> replacement{'r', 'e', 'p', 'l', 'a', 'c', 'e', 'd'};
  std::vector<uint8_t> replacementNode;
  ASSERT_TRUE(IPFS::CUnixFSJson::EncodeSingleFileNode(replacement.data(), replacement.size(),
                                                      replacementNode));

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(reinterpret_cast<const uint8_t*>(input.data()), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);
  ASSERT_TRUE(blockStore.Put(CBlock(parsed, replacementNode)));
  dataStore.Close();

  ASSERT_TRUE(ipfs.Initialize(path));

  std::vector<uint8_t> output;
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(output, replacement);

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, AddFileRejectsNullNonzero)
{
  const auto path = TempIPFSPath("null_nonzero");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid = "unchanged";
  EXPECT_FALSE(ipfs.AddFile(nullptr, 1, cid));
  EXPECT_EQ(cid, "unchanged");

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
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
  ASSERT_TRUE(ipfs.Initialize(path));

  std::vector<uint8_t> output{1, 2, 3};
  EXPECT_FALSE(ipfs.GetFile("not-a-cid", output));
  EXPECT_EQ(output, (std::vector<uint8_t>{1, 2, 3}));

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, PutDAGAndGetDAGRoundTrip)
{
  const auto path = TempIPFSPath("dag_round_trip");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

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
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, PutGetDagJsonStoresAndReturnsRawJson)
{
  const auto path = TempIPFSPath("dag_json_raw");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  CVariant content(CVariant::VariantTypeObject);
  content["hello"] = "world";

  std::string expectedJson;
  ASSERT_TRUE(CJSONVariantWriter::Write(content, expectedJson, true));

  const std::string cid = ipfs.PutDAG(content, CIDCodec::DAG_JSON);
  ASSERT_FALSE(cid.empty());

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::DAG_JSON);

  const CVariant output = ipfs.GetDAG(cid);
  EXPECT_TRUE(output.isObject());
  EXPECT_EQ(output["hello"].asString(), "world");

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);
  CBlock block;
  ASSERT_TRUE(blockStore.Get(parsed, block));
  EXPECT_EQ(std::string(reinterpret_cast<const char*>(block.Data()), block.Size()), expectedJson);
  dataStore.Close();

  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, PutGetDagJsonZstdStoresAndReturnsJson)
{
  const auto path = TempIPFSPath("dag_json_zstd");

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  CVariant content(CVariant::VariantTypeObject);
  content["hello"] = "compressed world";
  CVariant items(CVariant::VariantTypeArray);
  items.append(1);
  items.append(2);
  items.append(3);
  content["items"] = items;

  std::string expectedJson;
  ASSERT_TRUE(CJSONVariantWriter::Write(content, expectedJson, true));

  const std::string cid = ipfs.PutDAG(content, CIDCodec::DAG_JSON_ZSTD);
  ASSERT_FALSE(cid.empty());

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  EXPECT_EQ(parsed.Codec(), CIDCodec::DAG_JSON_ZSTD);

  const CVariant output = ipfs.GetDAG(cid);
  EXPECT_TRUE(output.isObject());
  EXPECT_EQ(output["hello"].asString(), "compressed world");
  ASSERT_TRUE(output["items"].isArray());
  EXPECT_EQ(output["items"].size(), 3U);
  EXPECT_EQ(output["items"][0].asInteger(), 1);
  EXPECT_EQ(output["items"][1].asInteger(), 2);
  EXPECT_EQ(output["items"][2].asInteger(), 3);

  ipfs.Deinitialize();

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);
  CBlock block;
  ASSERT_TRUE(blockStore.Get(parsed, block));
  EXPECT_NE(std::string(reinterpret_cast<const char*>(block.Data()), block.Size()), expectedJson);

  std::vector<uint8_t> decompressed;
  ASSERT_TRUE(IPFS::DecompressZstd(block.Data(), block.Size(), decompressed));
  EXPECT_EQ(std::string(reinterpret_cast<const char*>(decompressed.data()), decompressed.size()),
            expectedJson);
  dataStore.Close();

  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, GetDagJsonZstdRejectsInvalidCompressedBlock)
{
  const auto path = TempIPFSPath("dag_json_zstd_invalid");
  const std::vector<uint8_t> invalidBlock{0x7b, 0x22, 0x62, 0x61, 0x64, 0x22, 0x7d};

  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);
  CCID cid;
  ASSERT_TRUE(StoreTestBlock(blockStore, CIDCodec::DAG_JSON_ZSTD, invalidBlock, cid));
  dataStore.Close();

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));
  EXPECT_TRUE(ipfs.GetDAG(cid.ToString()).isNull());

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFS, DAGOperationsRejectOfflineAndInvalidCID)
{
  CIPFS ipfs;
  CVariant content(CVariant::VariantTypeObject);
  content["name"] = "Kodi";

  EXPECT_TRUE(ipfs.PutDAG(content).empty());
  EXPECT_TRUE(ipfs.GetDAG("not-a-cid").isNull());

  const auto path = TempIPFSPath("dag_invalid");
  ASSERT_TRUE(ipfs.Initialize(path));
  EXPECT_TRUE(ipfs.GetDAG("not-a-cid").isNull());

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}
