/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/ipfs/IPFSFile.h"
#include "filesystem/ipfs/IPFSService.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

TEST(TestIPFSFile, AddContentStoresBytesAndReturnsCID)
{
  CIPFSFile file;

  const uint8_t buffer[] = {'t', 'e', 's', 't'};
  std::string contentId = "unchanged";

  EXPECT_EQ(static_cast<ssize_t>(sizeof(buffer)),
            file.AddContent(buffer, sizeof(buffer), contentId));
  EXPECT_NE(contentId, "unchanged");
  EXPECT_FALSE(contentId.empty());
}

TEST(TestIPFSFile, InitForTestingInitializesSharedService)
{
  EXPECT_TRUE(CServiceBroker::GetIPFSService().IsInitialized());

  CIPFSFile file;
  const uint8_t buffer[] = {'t', 'e', 's', 't'};
  std::string contentId;

  EXPECT_EQ(static_cast<ssize_t>(sizeof(buffer)),
            file.AddContent(buffer, sizeof(buffer), contentId));
  EXPECT_FALSE(contentId.empty());
}

TEST(TestIPFSFile, AddContentRejectsNullNonzero)
{
  CIPFSFile file;

  std::string contentId = "unchanged";

  EXPECT_EQ(-1, file.AddContent(nullptr, 1, contentId));
  EXPECT_EQ("unchanged", contentId);
}

TEST(TestIPFSFile, AddContentAllowsEmptyFile)
{
  CIPFSFile file;

  std::string contentId = "unchanged";

  EXPECT_EQ(0, file.AddContent(nullptr, 0, contentId));
  EXPECT_NE(contentId, "unchanged");
  EXPECT_FALSE(contentId.empty());
}

TEST(TestIPFSFile, AddContentThenOpenAndRead)
{
  const std::string input = "test";

  CIPFSFile writer;

  std::string contentId;
  ASSERT_EQ(static_cast<ssize_t>(input.size()),
            writer.AddContent(input.data(), input.size(), contentId));
  ASSERT_FALSE(contentId.empty());

  CIPFSFile reader;
  ASSERT_TRUE(reader.Open(CURL("ipfs://" + contentId)));

  std::vector<char> output(input.size());
  ASSERT_EQ(static_cast<ssize_t>(output.size()), reader.Read(output.data(), output.size()));
  EXPECT_EQ(input, std::string(output.begin(), output.end()));

  EXPECT_EQ(0, reader.Read(output.data(), output.size()));
}

TEST(TestIPFSFile, SeekAndRead)
{
  const std::string input = "abcdef";

  CIPFSFile writer;

  std::string contentId;
  ASSERT_EQ(static_cast<ssize_t>(input.size()),
            writer.AddContent(input.data(), input.size(), contentId));

  CIPFSFile reader;
  ASSERT_TRUE(reader.Open(CURL("ipfs://" + contentId)));

  EXPECT_EQ(2, reader.Seek(2, SEEK_SET));

  char output[3] = {};
  ASSERT_EQ(3, reader.Read(output, sizeof(output)));
  EXPECT_EQ("cde", std::string(output, sizeof(output)));
}

TEST(TestIPFSFile, StatReportsSize)
{
  const std::string input = "hello";

  CIPFSFile writer;

  std::string contentId;
  ASSERT_EQ(static_cast<ssize_t>(input.size()),
            writer.AddContent(input.data(), input.size(), contentId));

  CIPFSFile reader;
  ASSERT_TRUE(reader.Open(CURL("ipfs://" + contentId)));

  struct __stat64 statBuffer;
  ASSERT_EQ(0, reader.Stat(&statBuffer));
  EXPECT_EQ(static_cast<int64_t>(input.size()), statBuffer.st_size);
}

TEST(TestIPFSFile, ReadFailsWhenClosed)
{
  CIPFSFile file;

  char buffer[4]{};

  EXPECT_EQ(-1, file.Read(buffer, sizeof(buffer)));
}

TEST(TestIPFSFile, SeekFailsWhenClosed)
{
  CIPFSFile file;

  EXPECT_EQ(-1, file.Seek(0, SEEK_SET));
}

TEST(TestIPFSFile, StatFailsWhenClosed)
{
  CIPFSFile file;

  struct __stat64 statBuffer;
  EXPECT_EQ(-1, file.Stat(&statBuffer));
}

TEST(TestIPFSFile, PositionAndLengthFailWhenClosed)
{
  CIPFSFile file;

  EXPECT_EQ(-1, file.GetPosition());
  EXPECT_EQ(-1, file.GetLength());
}

TEST(TestIPFSFile, OpenRejectsInvalidCID)
{
  CIPFSFile file;

  EXPECT_FALSE(file.Open(CURL("ipfs://bafkreiexample")));
}
