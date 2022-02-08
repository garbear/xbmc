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
#include "filesystem/Directory.h"
#include "filesystem/ipfs/IPFSService.h"
#include "filesystem/ipfs/block/IPFSBlockUtils.h"
#include "filesystem/ipfs/test/IPFSTestUtils.h"
#include "filesystem/ipfs/unixfs/UnixFSCodec.h"
#include "filesystem/ipfs/unixfs/UnixFSDirectory.h"
#include "filesystem/ipfs/unixfs/UnixFSImporter.h"
#include "filesystem/ipfs/unixfs/UnixFSJson.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;
using namespace XFILE;
using namespace XFILE::IPFS::TEST;

namespace
{
std::string TempIPFSPath(const std::string& name)
{
  const std::string path =
      URIUtils::AddFileToFolder("special://temp", "kodi_unixfs_directory_test_" + name);
  if (XFILE::CDirectory::Exists(path))
    XFILE::CDirectory::RemoveRecursive(path);
  return path;
}

bool StoreUnixFSRoot(CBlockStore& blockStore, const IPFS::UnixFSJsonNode& node, CCID& cid)
{
  std::vector<uint8_t> json;
  if (!IPFS::CUnixFSJson::EncodeNode(node, json))
    return false;

  CIDCodec codec = CIDCodec::UNKNOWN;
  std::vector<uint8_t> payload;
  if (!IPFS::UNIXFS::CCodec::EncodeRootPayload(json, payload, codec))
    return false;

  return IPFS::CIPFSBlockUtils::StoreAddressedBlock(blockStore, codec, payload.data(),
                                                    payload.size(), cid);
}

const IPFS::UNIXFS::DirectoryEntry* FindEntry(
    const std::vector<IPFS::UNIXFS::DirectoryEntry>& entries, const std::string& name)
{
  auto it = std::find_if(entries.begin(), entries.end(),
                         [&name](const IPFS::UNIXFS::DirectoryEntry& entry)
                         { return entry.name == name; });
  return it == entries.end() ? nullptr : &*it;
}

const CIPFSEntry* FindEntry(const std::vector<CIPFSEntry>& entries, const std::string& name)
{
  auto it = std::find_if(entries.begin(), entries.end(),
                         [&name](const CIPFSEntry& entry) { return entry.name == name; });
  return it == entries.end() ? nullptr : &*it;
}
} // namespace

TEST(TestUnixFSDirectory, IsDirectoryReturnsTrueForDirectoryRoot)
{
  const auto path = TempIPFSPath("is_directory_true");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::Directory;

  CCID cid;
  ASSERT_TRUE(StoreUnixFSRoot(blockStore, node, cid));

  EXPECT_TRUE(IPFS::UNIXFS::CDirectory::IsDirectory(blockStore, cid.ToString()));

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSDirectory, IsDirectoryReturnsFalseForFileRoot)
{
  const auto path = TempIPFSPath("is_directory_false");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  std::string cid;
  ASSERT_TRUE(IPFS::UNIXFS::CImporter::AddFile(blockStore, nullptr, 0, cid));

  EXPECT_FALSE(IPFS::UNIXFS::CDirectory::IsDirectory(blockStore, cid));

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSDirectory, ListDirectoryReturnsEntries)
{
  const auto path = TempIPFSPath("list_entries");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  const std::string alphaCid = MakeTestCID(CIDCodec::DAG_JSON, {1}).ToString();
  const std::string betaCid = MakeTestCID(CIDCodec::DAG_JSON, {2}).ToString();

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::Directory;
  node.links = {{alphaCid, "alpha", 0, 10}, {betaCid, "beta", 0, 20}};

  CCID cid;
  ASSERT_TRUE(StoreUnixFSRoot(blockStore, node, cid));

  std::vector<IPFS::UNIXFS::DirectoryEntry> entries;
  ASSERT_TRUE(IPFS::UNIXFS::CDirectory::ListDirectory(blockStore, cid.ToString(), entries));
  ASSERT_EQ(entries.size(), 2U);
  const IPFS::UNIXFS::DirectoryEntry* alpha = FindEntry(entries, "alpha");
  const IPFS::UNIXFS::DirectoryEntry* beta = FindEntry(entries, "beta");
  ASSERT_NE(alpha, nullptr);
  ASSERT_NE(beta, nullptr);
  EXPECT_EQ(alpha->cid, alphaCid);
  EXPECT_EQ(alpha->type, IPFS::UNIXFS::DirectoryEntryType::Unknown);
  EXPECT_EQ(alpha->totalSize, 10U);
  EXPECT_EQ(beta->cid, betaCid);
  EXPECT_EQ(beta->type, IPFS::UNIXFS::DirectoryEntryType::Unknown);
  EXPECT_EQ(beta->totalSize, 20U);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSDirectory, ListDirectoryKeepsMissingChildrenAsUnknown)
{
  const auto path = TempIPFSPath("list_missing_children");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  const std::string missingCid = MakeTestCID(CIDCodec::DAG_JSON, {9}).ToString();

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::Directory;
  node.links = {{missingCid, "missing", 0, 42}};

  CCID cid;
  ASSERT_TRUE(StoreUnixFSRoot(blockStore, node, cid));

  std::vector<IPFS::UNIXFS::DirectoryEntry> entries;
  ASSERT_TRUE(IPFS::UNIXFS::CDirectory::ListDirectory(blockStore, cid.ToString(), entries));
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0].cid, missingCid);
  EXPECT_EQ(entries[0].type, IPFS::UNIXFS::DirectoryEntryType::Unknown);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSDirectory, ListDirectoryClassifiesFileChild)
{
  const auto path = TempIPFSPath("list_file_child");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  const uint8_t data[] = {1, 2, 3};
  std::string childCid;
  ASSERT_TRUE(IPFS::UNIXFS::CImporter::AddFile(blockStore, data, sizeof(data), childCid));

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::Directory;
  node.links = {{childCid, "file.txt", 0, sizeof(data)}};

  CCID cid;
  ASSERT_TRUE(StoreUnixFSRoot(blockStore, node, cid));

  std::vector<IPFS::UNIXFS::DirectoryEntry> entries;
  ASSERT_TRUE(IPFS::UNIXFS::CDirectory::ListDirectory(blockStore, cid.ToString(), entries));
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0].type, IPFS::UNIXFS::DirectoryEntryType::File);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSDirectory, ListDirectoryClassifiesDirectoryChild)
{
  const auto path = TempIPFSPath("list_directory_child");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  IPFS::UnixFSJsonNode child;
  child.type = IPFS::UnixFSJsonNodeType::Directory;

  CCID childCid;
  ASSERT_TRUE(StoreUnixFSRoot(blockStore, child, childCid));

  IPFS::UnixFSJsonNode parent;
  parent.type = IPFS::UnixFSJsonNodeType::Directory;
  parent.links = {{childCid.ToString(), "folder", 0, 0}};

  CCID parentCid;
  ASSERT_TRUE(StoreUnixFSRoot(blockStore, parent, parentCid));

  std::vector<IPFS::UNIXFS::DirectoryEntry> entries;
  ASSERT_TRUE(IPFS::UNIXFS::CDirectory::ListDirectory(blockStore, parentCid.ToString(), entries));
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0].type, IPFS::UNIXFS::DirectoryEntryType::Directory);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSDirectory, ListDirectoryClassifiesRawChildAsFile)
{
  const auto path = TempIPFSPath("list_raw_child");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  const uint8_t data[] = {4, 5, 6};
  CCID childCid;
  ASSERT_TRUE(IPFS::CIPFSBlockUtils::StoreAddressedBlock(blockStore, CIDCodec::RAW, data,
                                                         sizeof(data), childCid));

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::Directory;
  node.links = {{childCid.ToString(), "raw.bin", 0, sizeof(data)}};

  CCID cid;
  ASSERT_TRUE(StoreUnixFSRoot(blockStore, node, cid));

  std::vector<IPFS::UNIXFS::DirectoryEntry> entries;
  ASSERT_TRUE(IPFS::UNIXFS::CDirectory::ListDirectory(blockStore, cid.ToString(), entries));
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0].type, IPFS::UNIXFS::DirectoryEntryType::File);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSDirectory, ListDirectoryKeepsMissingRawChildAsUnknown)
{
  const auto path = TempIPFSPath("list_missing_raw_child");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  const std::string missingCid = MakeTestCID(CIDCodec::RAW, {7, 8, 9}).ToString();

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::Directory;
  node.links = {{missingCid, "missing.raw", 0, 3}};

  CCID cid;
  ASSERT_TRUE(StoreUnixFSRoot(blockStore, node, cid));

  std::vector<IPFS::UNIXFS::DirectoryEntry> entries;
  ASSERT_TRUE(IPFS::UNIXFS::CDirectory::ListDirectory(blockStore, cid.ToString(), entries));
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0].type, IPFS::UNIXFS::DirectoryEntryType::Unknown);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSDirectory, ListDirectoryRejectsFileRoot)
{
  const auto path = TempIPFSPath("list_file_root");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  std::string cid;
  ASSERT_TRUE(IPFS::UNIXFS::CImporter::AddFile(blockStore, nullptr, 0, cid));

  std::vector<IPFS::UNIXFS::DirectoryEntry> entries{
      {"unchanged", "cid", IPFS::UNIXFS::DirectoryEntryType::Unknown, 1}};
  EXPECT_FALSE(IPFS::UNIXFS::CDirectory::ListDirectory(blockStore, cid, entries));
  EXPECT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0].name, "unchanged");

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestUnixFSDirectory, ListDirectoryLeavesOutputUnchangedOnInvalidRoot)
{
  const auto path = TempIPFSPath("list_unchanged_on_failure");
  CDataStore dataStore;
  ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
  CBlockStore blockStore(dataStore);

  std::vector<IPFS::UNIXFS::DirectoryEntry> entries{
      {"sentinel", "cid", IPFS::UNIXFS::DirectoryEntryType::File, 99}};

  EXPECT_FALSE(IPFS::UNIXFS::CDirectory::ListDirectory(blockStore, "not-a-cid", entries));
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0].name, "sentinel");
  EXPECT_EQ(entries[0].cid, "cid");
  EXPECT_EQ(entries[0].type, IPFS::UNIXFS::DirectoryEntryType::File);
  EXPECT_EQ(entries[0].totalSize, 99U);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFSService, IsDirectoryRecognizesUnixFSJsonDirectoryRoot)
{
  const auto path = TempIPFSPath("service_is_directory");
  std::string rootCid;
  {
    CDataStore dataStore;
    ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
    CBlockStore blockStore(dataStore);

    IPFS::UnixFSJsonNode node;
    node.type = IPFS::UnixFSJsonNodeType::Directory;

    CCID cid;
    ASSERT_TRUE(StoreUnixFSRoot(blockStore, node, cid));
    rootCid = cid.ToString();
    dataStore.Close();
  }

  CIPFSService service;
  ASSERT_TRUE(service.Initialize(path));
  EXPECT_TRUE(service.IsDirectory(rootCid));

  service.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestIPFSService, ListDirectoryClassifiesFileAndDirectoryChildren)
{
  const auto path = TempIPFSPath("service_list_directory");
  std::string rootCid;
  std::string fileCid;
  std::string folderCid;
  {
    CDataStore dataStore;
    ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
    CBlockStore blockStore(dataStore);

    const uint8_t data[] = {1, 2, 3};
    ASSERT_TRUE(IPFS::UNIXFS::CImporter::AddFile(blockStore, data, sizeof(data), fileCid));

    IPFS::UnixFSJsonNode folder;
    folder.type = IPFS::UnixFSJsonNodeType::Directory;
    CCID childDirectoryCid;
    ASSERT_TRUE(StoreUnixFSRoot(blockStore, folder, childDirectoryCid));
    folderCid = childDirectoryCid.ToString();

    IPFS::UnixFSJsonNode node;
    node.type = IPFS::UnixFSJsonNodeType::Directory;
    node.links = {{fileCid, "movie.mkv", 0, sizeof(data)}, {folderCid, "folder", 0, 0}};

    CCID cid;
    ASSERT_TRUE(StoreUnixFSRoot(blockStore, node, cid));
    rootCid = cid.ToString();
    dataStore.Close();
  }

  CIPFSService service;
  ASSERT_TRUE(service.Initialize(path));

  std::vector<CIPFSEntry> entries;
  ASSERT_TRUE(service.ListDirectory(rootCid, entries));
  ASSERT_EQ(entries.size(), 2U);

  const CIPFSEntry* file = FindEntry(entries, "movie.mkv");
  const CIPFSEntry* folder = FindEntry(entries, "folder");
  ASSERT_NE(file, nullptr);
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(file->cid, fileCid);
  EXPECT_EQ(file->size, 3U);
  EXPECT_TRUE(file->IsFile());
  EXPECT_EQ(folder->cid, folderCid);
  EXPECT_TRUE(folder->IsDirectory());

  service.Deinitialize();
  XFILE::CDirectory::RemoveRecursive(path);
}
