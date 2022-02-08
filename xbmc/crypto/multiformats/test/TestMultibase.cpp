/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/multiformats/Multibase.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

TEST(TestMultibase, TestTranslateEncoding)
{
  EXPECT_EQ('f', CMultibase::TranslateEncoding(CMultibase::Encoding::BASE16));
  EXPECT_EQ('m', CMultibase::TranslateEncoding(CMultibase::Encoding::BASE64));
}

TEST(TestMultibase, TestTranslateCharToEncoding)
{
  EXPECT_EQ(CMultibase::Encoding::BASE16, CMultibase::TranslateEncoding('f'));
  EXPECT_EQ(CMultibase::Encoding::BASE64, CMultibase::TranslateEncoding('m'));
}

TEST(TestMultibase, TestSetDataWithEncodingAndPayload)
{
  const uint8_t payload[] = {0x12, 0x34, 0x56};
  CMultibase multibase(CMultibase::Encoding::BASE16, payload, sizeof(payload));
  EXPECT_EQ(CMultibase::Encoding::BASE16, multibase.GetEncoding());
  EXPECT_EQ(4, multibase.GetDataSize());
  EXPECT_EQ('f', multibase.GetDataPtr()[0]);
}

TEST(TestMultibase, TestSetDataWithMultibaseString)
{
  std::string multibaseStr = "f123456";
  CMultibase multibase(multibaseStr);
  EXPECT_EQ(CMultibase::Encoding::BASE16, multibase.GetEncoding());
  EXPECT_EQ(7, multibase.GetDataSize());
  EXPECT_EQ('f', multibase.GetDataPtr()[0]);
}

TEST(TestMultibase, TestSetDataWithMultibaseRawData)
{
  const uint8_t data[] = {'f', 0x12, 0x34, 0x56};
  CMultibase multibase(data, sizeof(data));
  EXPECT_EQ(CMultibase::Encoding::BASE16, multibase.GetEncoding());
  EXPECT_EQ(4, multibase.GetDataSize());
}

TEST(TestMultibase, DISABLED_TestSerialization)
{
  const uint8_t payload[] = {0x12, 0x34, 0x56};
  CMultibase multibase(CMultibase::Encoding::BASE16, payload, sizeof(payload));
  EXPECT_EQ("f123456", multibase.Serialize());
}

TEST(TestMultibase, TestEmptyMultibaseString)
{
  std::string multibaseStr = "";
  CMultibase multibase(multibaseStr);
  EXPECT_EQ(CMultibase::Encoding::UNKNOWN, multibase.GetEncoding());
  EXPECT_EQ(0, multibase.GetDataSize());
}

TEST(TestMultibase, TestInvalidEncoding)
{
  std::string multibaseStr = "x123456"; // "x" isn't a known encoding
  CMultibase multibase(multibaseStr);
  EXPECT_EQ(CMultibase::Encoding::UNKNOWN, multibase.GetEncoding());
}

TEST(TestMultibase, TestEmptyPayload)
{
  std::vector<uint8_t> payload;
  CMultibase multibase(CMultibase::Encoding::BASE16, payload.data(), payload.size());
  EXPECT_EQ(CMultibase::Encoding::BASE16, multibase.GetEncoding());
  EXPECT_EQ(1, multibase.GetDataSize()); // Only encoding character should be there
  EXPECT_EQ('f', multibase.GetDataPtr()[0]);
}

TEST(TestMultibase, TestSetDataWithNullPointer)
{
  CMultibase multibase;
  multibase.SetData(nullptr, 0);
  EXPECT_EQ(CMultibase::Encoding::UNKNOWN, multibase.GetEncoding());
  EXPECT_EQ(0, multibase.GetDataSize());
}

TEST(TestMultibase, TestUnknownEncodingToString)
{
  EXPECT_EQ("unknown", CMultibase::ToString(CMultibase::Encoding::UNKNOWN));
}

TEST(TestMultibase, TestSetDataTwice)
{
  const uint8_t payload1[] = {0x12, 0x34};
  const uint8_t payload2[] = {0x56, 0x78};
  CMultibase multibase(CMultibase::Encoding::BASE16, payload1, sizeof(payload1));
  multibase.SetData(CMultibase::Encoding::BASE64, payload2, sizeof(payload2));
  EXPECT_EQ(CMultibase::Encoding::BASE64, multibase.GetEncoding());
  EXPECT_EQ(3, multibase.GetDataSize()); // Encoding character + payload2 size
  EXPECT_EQ('m', multibase.GetDataPtr()[0]);
}

TEST(TestMultibase, TestUnknownCharToEncoding)
{
  EXPECT_EQ(CMultibase::Encoding::UNKNOWN, CMultibase::TranslateEncoding('x'));
}

TEST(TestMultibase, DISABLED_TestSerializationWithUnknownEncoding)
{
  const uint8_t data[] = {'x', 0x12, 0x34, 0x56}; // "x" is an unknown encoding
  CMultibase multibase(data, sizeof(data));
  EXPECT_EQ("x123456", multibase.Serialize());
}

TEST(TestMultibase, TestMultibaseDataWithoutEncoding)
{
  const uint8_t data[] = {0x12, 0x34, 0x56}; // Missing encoding character
  CMultibase multibase(data, sizeof(data));
  EXPECT_EQ(CMultibase::Encoding::UNKNOWN, multibase.GetEncoding());
  EXPECT_EQ(3, multibase.GetDataSize());
}
