/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItemList.h"
#include "URL.h"
#include "filesystem/ipfs/IPFSDirectory.h"

#include <gtest/gtest.h>

using namespace XFILE;

TEST(TestIPFSDirectory, UnsupportedDirectoryOperationsFailCleanly)
{
  CIPFSDirectory directory;
  const CURL url("ipfs://bafkreiexample");
  CFileItemList items;

  EXPECT_FALSE(directory.GetDirectory(url, items));
  EXPECT_FALSE(directory.ContainsFiles(url));
  EXPECT_EQ(directory.GetCacheType(url), XFILE::CacheType::ALWAYS);
}
