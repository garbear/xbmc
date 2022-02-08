/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/ipfs/UnixFS.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

TEST(TestUnixFSFlatBuffer, SingleFileNodeRoundTrips)
{
  const std::string input = "inline flatbuffer file\n";

  std::vector<uint8_t> encoded;
  ASSERT_TRUE(XFILE::IPFS::CUnixFS::EncodeSingleFileNode(
      reinterpret_cast<const uint8_t*>(input.data()), input.size(), encoded));
  ASSERT_FALSE(encoded.empty());

  XFILE::IPFS::UnixFSNode decoded;
  ASSERT_TRUE(XFILE::IPFS::CUnixFS::DecodeNode(encoded.data(), encoded.size(), decoded));

  EXPECT_EQ(decoded.type, XFILE::IPFS::UnixFSNodeType::File);
  EXPECT_EQ(decoded.fileSize, input.size());
  EXPECT_EQ(input, std::string(decoded.data.begin(), decoded.data.end()));
}

TEST(TestUnixFSFlatBuffer, EmptyFileNodeRoundTrips)
{
  std::vector<uint8_t> encoded;
  ASSERT_TRUE(XFILE::IPFS::CUnixFS::EncodeSingleFileNode(nullptr, 0, encoded));
  ASSERT_FALSE(encoded.empty());

  XFILE::IPFS::UnixFSNode decoded;
  ASSERT_TRUE(XFILE::IPFS::CUnixFS::DecodeNode(encoded.data(), encoded.size(), decoded));

  EXPECT_EQ(decoded.type, XFILE::IPFS::UnixFSNodeType::File);
  EXPECT_EQ(decoded.fileSize, 0);
  EXPECT_TRUE(decoded.data.empty());
}

TEST(TestUnixFSFlatBuffer, EncodeRejectsNullNonzero)
{
  std::vector<uint8_t> encoded{1, 2, 3};
  EXPECT_FALSE(XFILE::IPFS::CUnixFS::EncodeSingleFileNode(nullptr, 1, encoded));
  EXPECT_EQ(encoded, (std::vector<uint8_t>{1, 2, 3}));
}

TEST(TestUnixFSFlatBuffer, DecodeRejectsMalformedInput)
{
  XFILE::IPFS::UnixFSNode node;

  EXPECT_FALSE(XFILE::IPFS::CUnixFS::DecodeNode(nullptr, 0, node));

  const std::vector<uint8_t> invalidMagic{'n', 'o', 'p', 'e'};
  EXPECT_FALSE(XFILE::IPFS::CUnixFS::DecodeNode(invalidMagic.data(), invalidMagic.size(), node));

  const std::string input = "truncated flatbuffer\n";
  std::vector<uint8_t> encoded;
  ASSERT_TRUE(XFILE::IPFS::CUnixFS::EncodeSingleFileNode(
      reinterpret_cast<const uint8_t*>(input.data()), input.size(), encoded));
  encoded.pop_back();

  EXPECT_FALSE(XFILE::IPFS::CUnixFS::DecodeNode(encoded.data(), encoded.size(), node));
}

TEST(TestUnixFSFlatBuffer, RejectsMalformedFileBlocksizeLinkMismatch)
{
  XFILE::IPFS::UnixFSNode node;
  node.type = XFILE::IPFS::UnixFSNodeType::File;
  node.fileSize = 0;
  node.blockSizes.push_back(1);

  EXPECT_FALSE(XFILE::IPFS::CUnixFS::ValidateNode(node));
}

TEST(TestUnixFSFlatBuffer, ValidateRejectsUnsupportedNodeTypesAndLinks)
{
  XFILE::IPFS::UnixFSNode node;

  node.type = XFILE::IPFS::UnixFSNodeType::Directory;
  EXPECT_FALSE(XFILE::IPFS::CUnixFS::ValidateNode(node));

  node.type = XFILE::IPFS::UnixFSNodeType::Symlink;
  EXPECT_FALSE(XFILE::IPFS::CUnixFS::ValidateNode(node));

  node.type = XFILE::IPFS::UnixFSNodeType::File;
  node.fileSize = 1;
  EXPECT_FALSE(XFILE::IPFS::CUnixFS::ValidateNode(node));

  node.fileSize = 0;
  node.links.push_back({});
  node.blockSizes.push_back(0);
  EXPECT_FALSE(XFILE::IPFS::CUnixFS::ValidateNode(node));
}
