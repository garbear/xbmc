/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/ipfs/IPFSUtils.h"

#include <string>

#include <gtest/gtest.h>

using namespace XFILE;

TEST(TestIPFSUtils, ParseCIDAcceptsIPFSProtocolURL)
{
  std::string cid;

  EXPECT_TRUE(CIPFSUtils::ParseCID(CURL("ipfs://bafkreiexample"), cid));
  EXPECT_EQ("bafkreiexample", cid);
}

TEST(TestIPFSUtils, ParseCIDAcceptsTrailingSlash)
{
  std::string cid;

  EXPECT_TRUE(CIPFSUtils::ParseCID(CURL("ipfs://bafkreiexample/"), cid));
  EXPECT_EQ("bafkreiexample", cid);
}

TEST(TestIPFSUtils, ParseCIDAcceptsIPFSPath)
{
  std::string cid;

  EXPECT_TRUE(CIPFSUtils::ParseCID(CURL("/ipfs/bafkreiexample"), cid));
  EXPECT_EQ("bafkreiexample", cid);
}

TEST(TestIPFSUtils, ParseCIDRejectsNestedPath)
{
  std::string cid = "unchanged";

  EXPECT_FALSE(CIPFSUtils::ParseCID(CURL("ipfs://bafkreiexample/file.txt"), cid));
  EXPECT_EQ("unchanged", cid);

  EXPECT_FALSE(CIPFSUtils::ParseCID(CURL("/ipfs/bafkreiexample/file.txt"), cid));
  EXPECT_EQ("unchanged", cid);
}

TEST(TestIPFSUtils, ParseCIDRejectsEmptyCID)
{
  std::string cid = "unchanged";

  EXPECT_FALSE(CIPFSUtils::ParseCID(CURL("ipfs://"), cid));
  EXPECT_EQ("unchanged", cid);

  EXPECT_FALSE(CIPFSUtils::ParseCID(CURL("/ipfs/"), cid));
  EXPECT_EQ("unchanged", cid);
}
