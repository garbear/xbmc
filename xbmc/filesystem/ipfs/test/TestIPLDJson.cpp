/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "datastore/CID.h"
#include "filesystem/ipfs/ipld/IPLDJson.h"
#include "filesystem/ipfs/test/IPFSTestUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;
using namespace XFILE::IPFS;
using namespace XFILE::IPFS::TEST;

namespace
{
CVariant ParseJson(const std::string& json)
{
  CVariant parsed;
  EXPECT_TRUE(CJSONVariantParser::Parse(json, parsed));
  return parsed;
}
} // namespace

TEST(TestIPLDJson, BytesEncodeEmpty)
{
  std::string json;
  IPLD::CJson::AppendBytes(json, {});
  EXPECT_EQ(json, "{\"/\":{\"bytes\":\"\"}}");
}

TEST(TestIPLDJson, BytesEncodeBinary)
{
  std::string json;
  IPLD::CJson::AppendBytes(json, {0, 1, 2});
  EXPECT_EQ(json, "{\"/\":{\"bytes\":\"AAEC\"}}");
}

TEST(TestIPLDJson, BytesDecodeRoundTrips)
{
  const std::vector<uint8_t> input{0, 1, 2, 255};
  std::string json;
  IPLD::CJson::AppendBytes(json, input);

  std::vector<uint8_t> output;
  ASSERT_TRUE(IPLD::CJson::DecodeBytes(ParseJson(json), output));
  EXPECT_EQ(output, input);
}

TEST(TestIPLDJson, BytesDecodeRejectsPlainString)
{
  std::vector<uint8_t> output;
  EXPECT_FALSE(IPLD::CJson::DecodeBytes(ParseJson("\"AAEC\""), output));
}

TEST(TestIPLDJson, BytesDecodeRejectsOuterExtraKeys)
{
  std::vector<uint8_t> output;
  EXPECT_FALSE(
      IPLD::CJson::DecodeBytes(ParseJson("{\"/\":{\"bytes\":\"\"},\"extra\":true}"), output));
}

TEST(TestIPLDJson, BytesDecodeRejectsInnerExtraKeys)
{
  std::vector<uint8_t> output;
  EXPECT_FALSE(
      IPLD::CJson::DecodeBytes(ParseJson("{\"/\":{\"bytes\":\"\",\"extra\":true}}"), output));
}

TEST(TestIPLDJson, BytesDecodeRejectsInvalidBase64)
{
  std::vector<uint8_t> output;
  EXPECT_FALSE(IPLD::CJson::DecodeBytes(ParseJson("{\"/\":{\"bytes\":\"*\"}}"), output));
}

TEST(TestIPLDJson, LinkEncode)
{
  const std::string cid = MakeTestCID(CIDCodec::RAW, {1, 2, 3}).ToString();
  std::string json;
  IPLD::CJson::AppendLink(json, cid);
  EXPECT_EQ(json, "{\"/\":\"" + cid + "\"}");
}

TEST(TestIPLDJson, LinkDecodeRoundTrips)
{
  const std::string input = MakeTestCID(CIDCodec::RAW, {1, 2, 3}).ToString();
  std::string json;
  IPLD::CJson::AppendLink(json, input);

  std::string output;
  ASSERT_TRUE(IPLD::CJson::DecodeLink(ParseJson(json), output));
  EXPECT_EQ(output, input);
}

TEST(TestIPLDJson, LinkDecodeRejectsPlainString)
{
  const std::string cid = MakeTestCID(CIDCodec::RAW, {1, 2, 3}).ToString();
  std::string output;
  EXPECT_FALSE(IPLD::CJson::DecodeLink(ParseJson("\"" + cid + "\""), output));
}

TEST(TestIPLDJson, LinkDecodeRejectsExtraKeys)
{
  const std::string cid = MakeTestCID(CIDCodec::RAW, {1, 2, 3}).ToString();
  std::string output;
  EXPECT_FALSE(
      IPLD::CJson::DecodeLink(ParseJson("{\"/\":\"" + cid + "\",\"extra\":true}"), output));
}

TEST(TestIPLDJson, LinkDecodeRejectsInvalidCID)
{
  std::string output;
  EXPECT_FALSE(IPLD::CJson::DecodeLink(ParseJson("{\"/\":\"not-a-cid\"}"), output));
}

TEST(TestIPLDJson, StringEscapingIsDeterministic)
{
  std::string json;
  IPLD::CJson::AppendString(json, std::string_view{"a\nb\"\\\x01", 6});
  EXPECT_EQ(json, "\"a\\nb\\\"\\\\\\u0001\"");
}
