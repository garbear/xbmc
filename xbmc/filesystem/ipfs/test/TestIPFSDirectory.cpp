/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "datastore/BlockStore.h"
#include "datastore/CID.h"
#include "datastore/DataStore.h"
#include "filesystem/Directory.h"
#include "filesystem/ipfs/IPFSDirectory.h"
#include "filesystem/ipfs/IPFSService.h"
#include "filesystem/ipfs/block/IPFSBlockUtils.h"
#include "filesystem/ipfs/test/IPFSTestUtils.h"
#include "filesystem/ipfs/unixfs/UnixFSCodec.h"
#include "filesystem/ipfs/unixfs/UnixFSImporter.h"
#include "filesystem/ipfs/unixfs/UnixFSJson.h"
#include "utils/URIUtils.h"

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
      URIUtils::AddFileToFolder("special://temp", "kodi_ipfs_directory_test_" + name);
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

bool StoreUnixFSRootAtPath(const std::string& path,
                           const IPFS::UnixFSJsonNode& node,
                           std::string& cid)
{
  CDataStore dataStore;
  if (!dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")))
    return false;

  CBlockStore blockStore(dataStore);
  CCID rootCid;
  if (!StoreUnixFSRoot(blockStore, node, rootCid))
    return false;

  cid = rootCid.ToString();
  dataStore.Close();
  return !cid.empty();
}

void InitializeSharedIPFSService(const std::string& path)
{
  static std::string previousPath;
  CIPFSService& service = CServiceBroker::GetIPFSService();
  service.Deinitialize();
  if (!previousPath.empty() && XFILE::CDirectory::Exists(previousPath))
    XFILE::CDirectory::RemoveRecursive(previousPath);

  ASSERT_TRUE(service.Initialize(path));
  previousPath = path;
}

CFileItemPtr FindItem(const CFileItemList& items, const std::string& label)
{
  for (int index = 0; index < items.Size(); ++index)
  {
    CFileItemPtr item = items[index];
    if (item && item->GetLabel() == label)
      return item;
  }

  return {};
}

bool HasTrailingSlash(const std::string& path)
{
  return !path.empty() && path.back() == '/';
}
} // namespace

TEST(TestIPFSDirectory, GetDirectoryReturnsUnixFSEntries)
{
  const auto path = TempIPFSPath("get_directory_entries");
  const std::string firstCid = MakeTestCID(CIDCodec::DAG_JSON, {1}).ToString();
  const std::string secondCid = MakeTestCID(CIDCodec::DAG_JSON, {2}).ToString();

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::Directory;
  node.links = {{firstCid, "alpha.mkv", 0, 10}, {secondCid, "beta.mkv", 0, 20}};

  std::string rootCid;
  ASSERT_TRUE(StoreUnixFSRootAtPath(path, node, rootCid));
  InitializeSharedIPFSService(path);

  CIPFSDirectory directory;
  CFileItemList items;
  ASSERT_TRUE(directory.GetDirectory(CURL("ipfs://" + rootCid), items));

  ASSERT_EQ(items.Size(), 2);
  CFileItemPtr alpha = FindItem(items, "alpha.mkv");
  CFileItemPtr beta = FindItem(items, "beta.mkv");
  ASSERT_NE(alpha, nullptr);
  ASSERT_NE(beta, nullptr);
  EXPECT_EQ(alpha->GetPath(), "ipfs://" + firstCid);
  EXPECT_EQ(alpha->GetSize(), 10);
  EXPECT_FALSE(alpha->IsFolder());
  EXPECT_EQ(beta->GetPath(), "ipfs://" + secondCid);
  EXPECT_EQ(beta->GetSize(), 20);
  EXPECT_FALSE(beta->IsFolder());
}

TEST(TestIPFSDirectory, GetDirectoryMarksDirectoryChildrenAsFolders)
{
  const auto path = TempIPFSPath("get_directory_folder_child");
  std::string rootCid;
  std::string childCid;
  {
    CDataStore dataStore;
    ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
    CBlockStore blockStore(dataStore);

    IPFS::UnixFSJsonNode child;
    child.type = IPFS::UnixFSJsonNodeType::Directory;
    CCID storedChildCid;
    ASSERT_TRUE(StoreUnixFSRoot(blockStore, child, storedChildCid));
    childCid = storedChildCid.ToString();

    IPFS::UnixFSJsonNode parent;
    parent.type = IPFS::UnixFSJsonNodeType::Directory;
    parent.links = {{childCid, "folder", 0, 0}};

    CCID storedRootCid;
    ASSERT_TRUE(StoreUnixFSRoot(blockStore, parent, storedRootCid));
    rootCid = storedRootCid.ToString();
    dataStore.Close();
  }
  InitializeSharedIPFSService(path);

  CIPFSDirectory directory;
  CFileItemList items;
  ASSERT_TRUE(directory.GetDirectory(CURL("ipfs://" + rootCid), items));
  ASSERT_EQ(items.Size(), 1);
  ASSERT_NE(items[0], nullptr);
  EXPECT_TRUE(items[0]->IsFolder());
  EXPECT_TRUE(HasTrailingSlash(items[0]->GetPath()));
  EXPECT_EQ(items[0]->GetPath(), "ipfs://" + childCid + "/");
}

TEST(TestIPFSDirectory, GetDirectoryMarksFileChildrenAsFiles)
{
  const auto path = TempIPFSPath("get_directory_file_child");
  std::string rootCid;
  std::string childCid;
  {
    CDataStore dataStore;
    ASSERT_TRUE(dataStore.Open(URIUtils::AddFileToFolder(path, "ipfs")));
    CBlockStore blockStore(dataStore);

    const uint8_t data[] = {1, 2, 3};
    ASSERT_TRUE(IPFS::UNIXFS::CImporter::AddFile(blockStore, data, sizeof(data), childCid));

    IPFS::UnixFSJsonNode parent;
    parent.type = IPFS::UnixFSJsonNodeType::Directory;
    parent.links = {{childCid, "file.txt", 0, sizeof(data)}};

    CCID storedRootCid;
    ASSERT_TRUE(StoreUnixFSRoot(blockStore, parent, storedRootCid));
    rootCid = storedRootCid.ToString();
    dataStore.Close();
  }
  InitializeSharedIPFSService(path);

  CIPFSDirectory directory;
  CFileItemList items;
  ASSERT_TRUE(directory.GetDirectory(CURL("ipfs://" + rootCid), items));
  ASSERT_EQ(items.Size(), 1);
  ASSERT_NE(items[0], nullptr);
  EXPECT_FALSE(items[0]->IsFolder());
  EXPECT_FALSE(HasTrailingSlash(items[0]->GetPath()));
  EXPECT_EQ(items[0]->GetPath(), "ipfs://" + childCid);
}

TEST(TestIPFSDirectory, GetDirectoryKeepsUnknownChildrenAsFiles)
{
  const auto path = TempIPFSPath("get_directory_unknown_child");
  const std::string childCid = MakeTestCID(CIDCodec::RAW, {9, 9, 9}).ToString();

  IPFS::UnixFSJsonNode parent;
  parent.type = IPFS::UnixFSJsonNodeType::Directory;
  parent.links = {{childCid, "unknown", 0, 3}};

  std::string rootCid;
  ASSERT_TRUE(StoreUnixFSRootAtPath(path, parent, rootCid));
  InitializeSharedIPFSService(path);

  CIPFSDirectory directory;
  CFileItemList items;
  ASSERT_TRUE(directory.GetDirectory(CURL("ipfs://" + rootCid), items));
  ASSERT_EQ(items.Size(), 1);
  ASSERT_NE(items[0], nullptr);
  EXPECT_FALSE(items[0]->IsFolder());
  EXPECT_FALSE(HasTrailingSlash(items[0]->GetPath()));
  EXPECT_EQ(items[0]->GetPath(), "ipfs://" + childCid);
}

TEST(TestIPFSDirectory, ContainsFilesReturnsTrueForNonEmptyDirectory)
{
  const auto path = TempIPFSPath("contains_non_empty_directory");

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::Directory;
  node.links = {{MakeTestCID(CIDCodec::DAG_JSON, {1}).ToString(), "child", 0, 1}};

  std::string rootCid;
  ASSERT_TRUE(StoreUnixFSRootAtPath(path, node, rootCid));
  InitializeSharedIPFSService(path);

  CIPFSDirectory directory;
  EXPECT_TRUE(directory.ContainsFiles(CURL("ipfs://" + rootCid)));
}

TEST(TestIPFSDirectory, ContainsFilesReturnsFalseForEmptyDirectory)
{
  const auto path = TempIPFSPath("contains_empty_directory");

  IPFS::UnixFSJsonNode node;
  node.type = IPFS::UnixFSJsonNodeType::Directory;

  std::string rootCid;
  ASSERT_TRUE(StoreUnixFSRootAtPath(path, node, rootCid));
  InitializeSharedIPFSService(path);

  CIPFSDirectory directory;
  EXPECT_FALSE(directory.ContainsFiles(CURL("ipfs://" + rootCid)));
}

TEST(TestIPFSDirectory, GetDirectoryRejectsFileRoot)
{
  const auto path = TempIPFSPath("rejects_file_root");
  InitializeSharedIPFSService(path);

  std::string cid;
  ASSERT_TRUE(CServiceBroker::GetIPFSService().AddFile(nullptr, 0, cid));

  CIPFSDirectory directory;
  CFileItemList items;
  EXPECT_FALSE(directory.GetDirectory(CURL("ipfs://" + cid), items));
  EXPECT_EQ(items.Size(), 0);
}

TEST(TestIPFSDirectory, GetDirectoryRejectsInvalidCID)
{
  CIPFSDirectory directory;
  CFileItemList items;

  EXPECT_FALSE(directory.GetDirectory(CURL("ipfs://not-a-cid"), items));
  EXPECT_EQ(items.Size(), 0);
}
