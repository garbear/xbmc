/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Multicodec.h"

#include "VarInt.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace CRYPTO;

namespace
{
// A uint64 varint can consume at most 10 bytes (ceil(64 / 7)).
constexpr size_t MAX_VARINT_LENGTH = 10;

bool DecodeVarIntWithConsumedBytes(const uint8_t* data,
                                   size_t dataSize,
                                   uint64_t& value,
                                   size_t& bytesRead)
{
  value = 0;
  bytesRead = 0;

  if (data == nullptr || dataSize == 0)
    return false;

  if (!VarInt::Decode(data, dataSize, value))
    return false;

  for (; bytesRead < dataSize && bytesRead < MAX_VARINT_LENGTH; ++bytesRead)
  {
    if ((data[bytesRead] & 0x80) == 0)
    {
      ++bytesRead;
      return true;
    }
  }

  return false;
}
} // namespace

bool CMulticodec::TranslateCode(Code codeEnum, uint64_t& code)
{
  switch (codeEnum)
  {
    case Code::IDENTITY:
    case Code::CIDV1:
    case Code::RAW:
    case Code::DAG_PB:
    case Code::DAG_CBOR:
    case Code::LIBP2P_KEY:
    case Code::ED25519_PUB:
    case Code::DAG_JSON:
    case Code::DAG_JSON_LZ4:
    case Code::DAG_JSON_ZSTD:
    case Code::ED25519_PRIV:
      code = static_cast<uint64_t>(codeEnum);
      return true;
    default:
      break;
  }

  return false;
}

CMulticodec::Code CMulticodec::TranslateCode(uint64_t code)
{
  switch (code)
  {
    case 0x00:
      return Code::IDENTITY;
    case 0x01:
      return Code::CIDV1;
    case 0x55:
      return Code::RAW;
    case 0x70:
      return Code::DAG_PB;
    case 0x71:
      return Code::DAG_CBOR;
    case 0x72:
      return Code::LIBP2P_KEY;
    case 0xed:
      return Code::ED25519_PUB;
    case 0x0129:
      return Code::DAG_JSON;
    case 0x012a:
      return Code::DAG_JSON_LZ4;
    case 0x012b:
      return Code::DAG_JSON_ZSTD;
    case 0x1300:
      return Code::ED25519_PRIV;
    default:
      break;
  }

  return Code::UNKNOWN;
}

std::string CMulticodec::ToString(Code code)
{
  switch (code)
  {
    case Code::IDENTITY:
      return "identity";
    case Code::CIDV1:
      return "cidv1";
    case Code::RAW:
      return "raw";
    case Code::DAG_PB:
      return "dag-pb";
    case Code::DAG_CBOR:
      return "dag-cbor";
    case Code::LIBP2P_KEY:
      return "libp2p-key";
    case Code::ED25519_PUB:
      return "ed25519-pub";
    case Code::DAG_JSON:
      return "dag-json";
    case Code::DAG_JSON_LZ4:
      return "dag-json-lz4";
    case Code::DAG_JSON_ZSTD:
      return "dag-json-zstd";
    case Code::ED25519_PRIV:
      return "ed25519-priv";
    default:
      break;
  }

  return "unknown";
}

CMulticodec::Code CMulticodec::FromString(const std::string& code)
{
  if (code == "identity")
    return Code::IDENTITY;
  if (code == "cidv1")
    return Code::CIDV1;
  if (code == "raw")
    return Code::RAW;
  if (code == "dag-pb")
    return Code::DAG_PB;
  if (code == "dag-cbor")
    return Code::DAG_CBOR;
  if (code == "libp2p-key")
    return Code::LIBP2P_KEY;
  if (code == "ed25519-pub")
    return Code::ED25519_PUB;
  if (code == "dag-json")
    return Code::DAG_JSON;
  if (code == "dag-json-lz4")
    return Code::DAG_JSON_LZ4;
  if (code == "dag-json-zstd")
    return Code::DAG_JSON_ZSTD;
  if (code == "ed25519-priv")
    return Code::ED25519_PRIV;

  return Code::UNKNOWN;
}

bool CMulticodec::Encode(Code code, std::vector<uint8_t>& encoded)
{
  uint64_t codeValue = 0;
  if (!TranslateCode(code, codeValue))
    return false;

  std::vector<uint8_t> result(VarInt::EncodingLength(codeValue));
  if (!VarInt::EncodeTo(codeValue, result.data(), result.size()))
    return false;

  encoded = std::move(result);
  return true;
}

bool CMulticodec::EncodePrefix(Code code,
                               const uint8_t* payload,
                               size_t payloadSize,
                               std::vector<uint8_t>& prefixed)
{
  if (payload == nullptr && payloadSize != 0)
  {
    CLog::Log(LOGERROR, "CMulticodec::EncodePrefix - invalid null payload pointer with size {}",
              payloadSize);
    return false;
  }

  std::vector<uint8_t> encoded;
  if (!Encode(code, encoded))
    return false;

  encoded.reserve(encoded.size() + payloadSize);
  if (payloadSize != 0)
    encoded.insert(encoded.end(), payload, payload + payloadSize);

  prefixed = std::move(encoded);
  return true;
}

bool CMulticodec::Decode(const uint8_t* data, size_t dataSize, Code& code, size_t& bytesRead)
{
  uint64_t codeValue = 0;
  size_t consumed = 0;
  if (!DecodeVarIntWithConsumedBytes(data, dataSize, codeValue, consumed))
    return false;

  Code decoded = TranslateCode(codeValue);
  if (decoded == Code::UNKNOWN)
    return false;

  code = decoded;
  bytesRead = consumed;
  return true;
}

bool CMulticodec::DecodePrefix(const uint8_t* data,
                               size_t dataSize,
                               Code& code,
                               std::vector<uint8_t>& payload)
{
  Code decoded = Code::UNKNOWN;
  size_t bytesRead = 0;
  if (!Decode(data, dataSize, decoded, bytesRead))
    return false;

  std::vector<uint8_t> decodedPayload;
  decodedPayload.assign(data + bytesRead, data + dataSize);

  code = decoded;
  payload = std::move(decodedPayload);
  return true;
}
