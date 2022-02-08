/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/multiformats/Multicodec.h"

#include <stdint.h>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

TEST(TestMulticodec, TranslatesKnownTableCodes)
{
  uint64_t code = 0;

  EXPECT_TRUE(CMulticodec::TranslateCode(CMulticodec::Code::RAW, code));
  EXPECT_EQ(code, 0x55U);
  EXPECT_EQ(CMulticodec::Code::RAW, CMulticodec::TranslateCode(0x55));

  EXPECT_TRUE(CMulticodec::TranslateCode(CMulticodec::Code::DAG_JSON, code));
  EXPECT_EQ(code, 0x0129U);
  EXPECT_EQ(CMulticodec::Code::DAG_JSON, CMulticodec::TranslateCode(0x0129));

  EXPECT_TRUE(CMulticodec::TranslateCode(CMulticodec::Code::ED25519_PRIV, code));
  EXPECT_EQ(code, 0x1300U);
  EXPECT_EQ(CMulticodec::Code::ED25519_PRIV, CMulticodec::TranslateCode(0x1300));
}

TEST(TestMulticodec, DAGJsonZstdRoundTrips)
{
  uint64_t code = 0;
  ASSERT_TRUE(CMulticodec::TranslateCode(CMulticodec::Code::DAG_JSON_ZSTD, code));
  EXPECT_EQ(code, 0x012bU);
  EXPECT_EQ(CMulticodec::Code::DAG_JSON_ZSTD, CMulticodec::TranslateCode(0x012b));
  EXPECT_EQ(CMulticodec::ToString(CMulticodec::Code::DAG_JSON_ZSTD), "dag-json-zstd");
  EXPECT_EQ(CMulticodec::FromString("dag-json-zstd"), CMulticodec::Code::DAG_JSON_ZSTD);

  std::vector<uint8_t> encoded;
  ASSERT_TRUE(CMulticodec::Encode(CMulticodec::Code::DAG_JSON_ZSTD, encoded));
  EXPECT_EQ(encoded, (std::vector<uint8_t>{0xab, 0x02}));
}

TEST(TestMulticodec, StringTranslationHandlesKnownAndUnknownValues)
{
  EXPECT_EQ(CMulticodec::FromString("dag-json-zstd"), CMulticodec::Code::DAG_JSON_ZSTD);
  EXPECT_EQ(CMulticodec::FromString("dag-flatbuffer"), CMulticodec::Code::UNKNOWN);
  EXPECT_EQ(CMulticodec::FromString("dag-flatbuffer-zstd"), CMulticodec::Code::UNKNOWN);
  EXPECT_EQ(CMulticodec::FromString("not-a-codec"), CMulticodec::Code::UNKNOWN);
  EXPECT_EQ(CMulticodec::ToString(CMulticodec::Code::UNKNOWN), "unknown");
}

TEST(TestMulticodec, EncodesCodeAsUnsignedVarint)
{
  std::vector<uint8_t> encoded;

  ASSERT_TRUE(CMulticodec::Encode(CMulticodec::Code::RAW, encoded));
  EXPECT_EQ(encoded, (std::vector<uint8_t>{0x55}));

  ASSERT_TRUE(CMulticodec::Encode(CMulticodec::Code::ED25519_PUB, encoded));
  EXPECT_EQ(encoded, (std::vector<uint8_t>{0xed, 0x01}));

  ASSERT_TRUE(CMulticodec::Encode(CMulticodec::Code::ED25519_PRIV, encoded));
  EXPECT_EQ(encoded, (std::vector<uint8_t>{0x80, 0x26}));
}

TEST(TestMulticodec, PrefixRoundTripsPayload)
{
  const std::vector<uint8_t> payload{0x01, 0x02, 0x03};
  std::vector<uint8_t> prefixed;

  ASSERT_TRUE(CMulticodec::EncodePrefix(CMulticodec::Code::DAG_JSON, payload.data(), payload.size(),
                                        prefixed));
  EXPECT_EQ(prefixed, (std::vector<uint8_t>{0xa9, 0x02, 0x01, 0x02, 0x03}));

  CMulticodec::Code code = CMulticodec::Code::UNKNOWN;
  std::vector<uint8_t> decodedPayload;
  ASSERT_TRUE(CMulticodec::DecodePrefix(prefixed.data(), prefixed.size(), code, decodedPayload));
  EXPECT_EQ(code, CMulticodec::Code::DAG_JSON);
  EXPECT_EQ(decodedPayload, payload);
}

TEST(TestMulticodec, DecodeReportsConsumedPrefixBytes)
{
  const std::vector<uint8_t> prefixed{0xed, 0x01, 0xaa, 0xbb};
  CMulticodec::Code code = CMulticodec::Code::UNKNOWN;
  size_t bytesRead = 0;

  ASSERT_TRUE(CMulticodec::Decode(prefixed.data(), prefixed.size(), code, bytesRead));
  EXPECT_EQ(code, CMulticodec::Code::ED25519_PUB);
  EXPECT_EQ(bytesRead, 2U);
}

TEST(TestMulticodec, InvalidInputsLeaveOutputUnchanged)
{
  std::vector<uint8_t> prefixed{0x01, 0x02};
  EXPECT_FALSE(CMulticodec::EncodePrefix(CMulticodec::Code::RAW, nullptr, 1, prefixed));
  EXPECT_EQ(prefixed, (std::vector<uint8_t>{0x01, 0x02}));

  CMulticodec::Code code = CMulticodec::Code::RAW;
  size_t bytesRead = 4;
  EXPECT_FALSE(CMulticodec::Decode(nullptr, 1, code, bytesRead));
  EXPECT_EQ(code, CMulticodec::Code::RAW);
  EXPECT_EQ(bytesRead, 4U);

  const std::vector<uint8_t> unknown{0xff, 0xff, 0x01};
  EXPECT_FALSE(CMulticodec::Decode(unknown.data(), unknown.size(), code, bytesRead));
  EXPECT_EQ(code, CMulticodec::Code::RAW);
  EXPECT_EQ(bytesRead, 4U);
}
