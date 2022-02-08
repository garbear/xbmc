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
#include "filesystem/ipfs/ZstdCompression.h"
#include "filesystem/ipfs/test/IPFSTestUtils.h"
#include "filesystem/ipfs/unixfs/UnixFSJson.h"
#include "utils/URIUtils.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;
using namespace XFILE;
using namespace XFILE::IPFS;
using namespace XFILE::IPFS::TEST;

namespace
{
std::string TempIPFSPath(const std::string& name)
{
  const std::string path =
      URIUtils::AddFileToFolder("special://temp", "kodi_unixfs_json_test_" + name);
  if (XFILE::CDirectory::Exists(path))
    XFILE::CDirectory::RemoveRecursive(path);
  return path;
}

} // namespace

TEST(TestUnixFSJson, SingleFileNodeRoundTrips)
{
  const std::string input = "hello unixfs json\n";
  std::vector<uint8_t> encoded;
  ASSERT_TRUE(CUnixFSJson::EncodeSingleFileNode(reinterpret_cast<const uint8_t*>(input.data()),
                                                input.size(), encoded));

  UnixFSJsonNode node;
  ASSERT_TRUE(CUnixFSJson::DecodeNode(encoded.data(), encoded.size(), node));
  EXPECT_EQ(node.type, UnixFSJsonNodeType::File);
  EXPECT_EQ(node.fileSize, input.size());
  EXPECT_EQ(input, std::string(node.data.begin(), node.data.end()));
  EXPECT_TRUE(node.links.empty());
}

TEST(TestUnixFSJson, EmptyFileNodeRoundTrips)
{
  std::vector<uint8_t> encoded;
  ASSERT_TRUE(CUnixFSJson::EncodeSingleFileNode(nullptr, 0, encoded));

  UnixFSJsonNode node;
  ASSERT_TRUE(CUnixFSJson::DecodeNode(encoded.data(), encoded.size(), node));
  EXPECT_EQ(node.type, UnixFSJsonNodeType::File);
  EXPECT_EQ(node.fileSize, 0U);
  EXPECT_TRUE(node.data.empty());
  EXPECT_TRUE(node.links.empty());
}

TEST(TestUnixFSJson, EncodedInlineFileJsonIsDeterministic)
{
  const std::string input = "hello";
  std::vector<uint8_t> encoded;
  ASSERT_TRUE(CUnixFSJson::EncodeSingleFileNode(reinterpret_cast<const uint8_t*>(input.data()),
                                                input.size(), encoded));

  const std::string json(encoded.begin(), encoded.end());
  EXPECT_EQ(json, "{\"type\":\"File\",\"data\":{\"/\":{\"bytes\":\"aGVsbG8=\"}},\"filesize\":5,"
                  "\"links\":[],\"mode\":0,\"mtime\":{\"seconds\":0,\"nanos\":0}}");
}

TEST(TestUnixFSJson, EncodedDirectoryJsonSortsLinksDeterministically)
{
  const std::string alphaCid = MakeTestCID(CIDCodec::DAG_JSON, {1, 2, 3}).ToString();
  const std::string betaCid = MakeTestCID(CIDCodec::DAG_JSON, {4, 5, 6}).ToString();

  UnixFSJsonNode node;
  node.type = UnixFSJsonNodeType::Directory;
  node.links = {{betaCid, "beta", 0, 20}, {alphaCid, "alpha", 0, 10}};

  std::vector<uint8_t> encoded;
  ASSERT_TRUE(CUnixFSJson::EncodeNode(node, encoded));

  const std::string json(encoded.begin(), encoded.end());
  EXPECT_EQ(json, "{\"type\":\"Directory\",\"links\":[{\"name\":\"alpha\",\"link\":{\"/\":\"" +
                      alphaCid + "\"},\"tSize\":10},{\"name\":\"beta\",\"link\":{\"/\":\"" +
                      betaCid +
                      "\"},\"tSize\":20}],\"mode\":0,\"mtime\":{\"seconds\":0,"
                      "\"nanos\":0}}");
}

TEST(TestUnixFSJson, EncodedChunkedFileJsonIsDeterministic)
{
  const std::string firstCid = MakeTestCID(CIDCodec::RAW, {1, 2, 3}).ToString();
  const std::string secondCid = MakeTestCID(CIDCodec::RAW, {4, 5, 6}).ToString();

  UnixFSJsonNode node;
  node.type = UnixFSJsonNodeType::File;
  node.fileSize = 6;
  node.links = {{firstCid, "", 3, 3}, {secondCid, "", 3, 3}};

  std::vector<uint8_t> encoded;
  ASSERT_TRUE(CUnixFSJson::EncodeNode(node, encoded));

  const std::string json(encoded.begin(), encoded.end());
  EXPECT_EQ(json, "{\"type\":\"File\",\"data\":{\"/\":{\"bytes\":\"\"}},\"filesize\":6,\"links\":["
                  "{\"link\":{\"/\":\"" +
                      firstCid + "\"},\"blockSize\":3,\"tSize\":3},{\"link\":{\"/\":\"" +
                      secondCid +
                      "\"},\"blockSize\":3,\"tSize\":3}],\"mode\":0,\"mtime\":{\"seconds\":0,"
                      "\"nanos\":0}}");
}

TEST(TestUnixFSJson, EncodeRejectsNullNonzero)
{
  std::vector<uint8_t> encoded{1, 2, 3};
  EXPECT_FALSE(CUnixFSJson::EncodeSingleFileNode(nullptr, 1, encoded));
  EXPECT_EQ(encoded, (std::vector<uint8_t>{1, 2, 3}));
}

TEST(TestUnixFSJson, DecodeRejectsMalformedJson)
{
  const std::string malformed = "{\"type\":\"file\"";
  UnixFSJsonNode node;
  node.fileSize = 42;
  node.data = {4, 2};

  EXPECT_FALSE(CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(malformed.data()),
                                       malformed.size(), node));
  EXPECT_EQ(node.fileSize, 42U);
  EXPECT_EQ(node.data, (std::vector<uint8_t>{4, 2}));
}

TEST(TestUnixFSJson, DecodeRejectsMissingRequiredFields)
{
  const std::string missingLinks =
      "{\"type\":\"File\",\"data\":{\"/\":{\"bytes\":\"aGk=\"}},\"filesize\":2,\"mode\":0,"
      "\"mtime\":{\"seconds\":0,\"nanos\":0}}";
  UnixFSJsonNode node;
  EXPECT_FALSE(CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(missingLinks.data()),
                                       missingLinks.size(), node));

  const std::string missingMtime = "{\"type\":\"Directory\",\"links\":[],\"mode\":0}";
  EXPECT_FALSE(CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(missingMtime.data()),
                                       missingMtime.size(), node));
}

TEST(TestUnixFSJson, DecodeRejectsOldPlainDataAndLowercaseType)
{
  const std::string plainData =
      "{\"type\":\"File\",\"data\":\"aGk=\",\"filesize\":2,\"links\":[],\"mode\":0,"
      "\"mtime\":{\"seconds\":0,\"nanos\":0}}";
  UnixFSJsonNode node;
  EXPECT_FALSE(CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(plainData.data()),
                                       plainData.size(), node));

  const std::string lowercaseType =
      "{\"type\":\"file\",\"data\":{\"/\":{\"bytes\":\"aGk=\"}},\"filesize\":2,\"links\":[],"
      "\"mode\":0,\"mtime\":{\"seconds\":0,\"nanos\":0}}";
  EXPECT_FALSE(CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(lowercaseType.data()),
                                       lowercaseType.size(), node));
}

TEST(TestUnixFSJson, DecodeRejectsInvalidBase64Data)
{
  const std::string invalidBase64 =
      "{\"type\":\"File\",\"data\":{\"/\":{\"bytes\":\"not canonical\"}},\"filesize\":1,"
      "\"links\":[],\"mode\":0,"
      "\"mtime\":{\"seconds\":0,\"nanos\":0}}";
  UnixFSJsonNode node;
  EXPECT_FALSE(CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(invalidBase64.data()),
                                       invalidBase64.size(), node));
}

TEST(TestUnixFSJson, DecodeRejectsUnknownType)
{
  const std::string unknownType =
      "{\"type\":\"ham\",\"data\":{\"/\":{\"bytes\":\"\"}},\"filesize\":0,\"links\":[],\"mode\":0,"
      "\"mtime\":{\"seconds\":0,\"nanos\":0}}";
  UnixFSJsonNode node;
  EXPECT_FALSE(CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(unknownType.data()),
                                       unknownType.size(), node));
}

TEST(TestUnixFSJson, DecodeRejectsInvalidLinkShape)
{
  const std::string cid = MakeTestCID(CIDCodec::RAW, {1}).ToString();
  const std::string fileLinkWithName =
      "{\"type\":\"File\",\"data\":{\"/\":{\"bytes\":\"\"}},\"filesize\":1,"
      "\"links\":[{\"name\":\"chunk\",\"link\":{\"/\":\"" +
      cid +
      "\"},\"blockSize\":1,\"tSize\":1}],\"mode\":0,"
      "\"mtime\":{\"seconds\":0,\"nanos\":0}}";
  UnixFSJsonNode node;
  EXPECT_FALSE(CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(fileLinkWithName.data()),
                                       fileLinkWithName.size(), node));

  const std::string fileLinkWithCid =
      "{\"type\":\"File\",\"data\":{\"/\":{\"bytes\":\"\"}},\"filesize\":1,"
      "\"links\":[{\"cid\":\"" +
      cid + "\",\"blockSize\":1,\"tSize\":1}],\"mode\":0,\"mtime\":{\"seconds\":0,\"nanos\":0}}";
  EXPECT_FALSE(CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(fileLinkWithCid.data()),
                                       fileLinkWithCid.size(), node));

  const std::string directoryLinkWithBlockSize =
      "{\"type\":\"Directory\",\"links\":[{\"name\":\"child\",\"link\":{\"/\":\"" +
      MakeTestCID(CIDCodec::DAG_JSON, {1}).ToString() +
      "\"},\"blockSize\":1,\"tSize\":1}],\"mode\":0,\"mtime\":{\"seconds\":0,\"nanos\":0}}";
  EXPECT_FALSE(
      CUnixFSJson::DecodeNode(reinterpret_cast<const uint8_t*>(directoryLinkWithBlockSize.data()),
                              directoryLinkWithBlockSize.size(), node));
}

TEST(TestUnixFSJson, ValidateNodeInvariants)
{
  UnixFSJsonNode file;
  file.type = UnixFSJsonNodeType::File;
  file.data = {'a', 'b'};
  file.fileSize = 3;
  EXPECT_FALSE(CUnixFSJson::ValidateNode(file));

  file.fileSize = 2;
  EXPECT_TRUE(CUnixFSJson::ValidateNode(file));

  UnixFSJsonNode directory;
  directory.type = UnixFSJsonNodeType::Directory;
  directory.links = {{MakeTestCID(CIDCodec::DAG_JSON, {1}).ToString(), "same", 0, 10},
                     {MakeTestCID(CIDCodec::DAG_JSON, {2}).ToString(), "same", 0, 20}};
  EXPECT_FALSE(CUnixFSJson::ValidateNode(directory));

  directory.links[1].name = "other";
  EXPECT_TRUE(CUnixFSJson::ValidateNode(directory));

  UnixFSJsonNode symlink;
  symlink.type = UnixFSJsonNodeType::Symlink;
  symlink.data = {'t', 'a', 'r', 'g', 'e', 't'};
  symlink.fileSize = symlink.data.size();
  EXPECT_TRUE(CUnixFSJson::ValidateNode(symlink));
}

TEST(TestUnixFSJson, ValidateFileRequiresChunkCids)
{
  UnixFSJsonNode node;
  node.type = UnixFSJsonNodeType::File;
  node.fileSize = 1;
  node.links = {{"", "", 1, 1}};

  EXPECT_FALSE(CUnixFSJson::ValidateNode(node));
}

TEST(TestUnixFSJson, ValidateFileRequiresChunkBlockSizes)
{
  UnixFSJsonNode node;
  node.type = UnixFSJsonNodeType::File;
  node.fileSize = 0;
  node.links = {{MakeTestCID(CIDCodec::RAW, {1}).ToString(), "", 0, 0}};

  EXPECT_FALSE(CUnixFSJson::ValidateNode(node));
}

TEST(TestUnixFSJson, ZstdNodeRoundTrips)
{
  const auto path = TempIPFSPath("zstd_round_trip");
  const std::vector<uint8_t> input(32 * 1024, 'A');

  CIPFS ipfs;
  ASSERT_TRUE(ipfs.Initialize(path));

  std::string cid;
  ASSERT_TRUE(ipfs.AddFile(input.data(), input.size(), cid));

  CCID parsed;
  ASSERT_TRUE(CCID::FromString(cid, parsed));
  ASSERT_EQ(parsed.Codec(), CIDCodec::DAG_JSON_ZSTD);

  std::vector<uint8_t> output;
  ASSERT_TRUE(ipfs.GetFile(cid, output));
  EXPECT_EQ(output, input);

  ipfs.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSJson, ZstdCIDHashesCompressedBytes)
{
  const auto path = TempIPFSPath("zstd_hashes_compressed");
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

  std::vector<uint8_t> decompressed;
  ASSERT_TRUE(DecompressZstd(block.Data(), block.Size(), decompressed));
  EXPECT_NE(parsed.Multihash(), MakeSha2_256Multihash(decompressed.data(), decompressed.size()));

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSJson, ZstdRejectsMalformedCompressedBytes)
{
  const std::vector<uint8_t> malformed{'n', 'o', 't', ' ', 'z', 's', 't', 'd'};
  std::vector<uint8_t> output{1, 2, 3};
  EXPECT_FALSE(DecompressZstd(malformed.data(), malformed.size(), output));
  EXPECT_EQ(output, (std::vector<uint8_t>{1, 2, 3}));
}

TEST(TestUnixFSJson, ZstdRejectsMalformedJsonAfterDecompress)
{
  const std::string malformedJson = "{\"type\":\"file\"";
  std::vector<uint8_t> compressed;
  ASSERT_TRUE(CompressZstd(reinterpret_cast<const uint8_t*>(malformedJson.data()),
                           malformedJson.size(), compressed));

  std::vector<uint8_t> decompressed;
  ASSERT_TRUE(DecompressZstd(compressed.data(), compressed.size(), decompressed));

  UnixFSJsonNode node;
  EXPECT_FALSE(CUnixFSJson::DecodeNode(decompressed.data(), decompressed.size(), node));
}
