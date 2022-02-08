/*
 *  Copyright (C) 2018-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CID.h"

#include "crypto/codecs/Base58BTC.h"
#include "crypto/multiformats/Multibase.h"
#include "crypto/multiformats/Multicodec.h"
#include "crypto/multiformats/VarInt.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace DATASTORE;

namespace
{
constexpr uint64_t CID_VERSION = 1;
} // namespace

CCID::CCID(const CCID& cid)
{
  if (cid.Size() == 0)
  {
    SetOwnedBytes({});
    return;
  }

  if (cid.Data() == nullptr)
  {
    CLog::Log(LOGERROR, "CID: Cannot copy {} bytes from a null internal data pointer", cid.Size());
    SetOwnedBytes({});
    return;
  }

  SetOwnedBytes(std::vector<uint8_t>(cid.Data(), cid.Data() + cid.Size()));
}

CCID::CCID(CIDCodec codec, std::vector<uint8_t> multihash)
{
  m_codec = codec;
  m_multihash = std::move(multihash);
  SetOwnedBytes(SerializeCID(m_codec, m_multihash));
}

CCID::CCID(std::vector<uint8_t> bytes)
{
  SetOwnedBytes(std::move(bytes));
}

CCID CCID::FromBytes(const uint8_t* data, size_t size)
{
  CCID cid;
  cid.Deserialize(data, size);
  return cid;
}

CCID CCID::ViewBytes(const uint8_t* data, size_t size)
{
  CCID cid;
  cid.SetViewBytes(data, size);
  return cid;
}

CCID& CCID::operator=(const CCID& cid)
{
  if (this == &cid)
    return *this;

  if (cid.Size() == 0)
  {
    SetOwnedBytes({});
    return *this;
  }

  if (cid.Data() == nullptr)
  {
    CLog::Log(LOGERROR, "CID: Cannot copy-assign {} bytes from a null internal data pointer",
              cid.Size());
    SetOwnedBytes({});
    return *this;
  }

  SetOwnedBytes(std::vector<uint8_t>(cid.Data(), cid.Data() + cid.Size()));
  return *this;
}

bool CCID::operator==(const CCID& other) const
{
  if (Size() != other.Size())
    return false;

  if (Size() == 0)
    return true;

  return std::equal(Data(), Data() + Size(), other.Data());
}

const uint8_t* CCID::Data() const
{
  if (m_storageMode == StorageMode::View)
    return m_viewData;

  return m_bytes.data();
}

size_t CCID::Size() const
{
  if (m_storageMode == StorageMode::View)
    return m_viewSize;

  return m_bytes.size();
}

bool CCID::IsOwning() const
{
  return m_storageMode == StorageMode::Owning;
}

void CCID::SetCodec(CIDCodec codec)
{
  m_codec = codec;
  SetOwnedBytes(SerializeCID(m_codec, m_multihash));
}

void CCID::SetMultihash(std::vector<uint8_t> multihash)
{
  m_multihash = std::move(multihash);
  SetOwnedBytes(SerializeCID(m_codec, m_multihash));
}

std::pair<const uint8_t*, size_t> CCID::Serialize() const
{
  return std::make_pair(Data(), Size());
}

std::string CCID::ToString() const
{
  if (Empty() || Data() == nullptr)
    return {};

  const char base58BtcPrefix = static_cast<char>(
      CRYPTO::CMultibase::TranslateEncoding(CRYPTO::CMultibase::Encoding::BASE58BTC));

  std::string encoded;
  encoded.push_back(base58BtcPrefix);
  encoded += CRYPTO::CBase58BTC::EncodeBase58(Data(), Size());
  return encoded;
}

bool CCID::FromString(const std::string& cidString, CCID& cid)
{
  const char base58BtcPrefix = static_cast<char>(
      CRYPTO::CMultibase::TranslateEncoding(CRYPTO::CMultibase::Encoding::BASE58BTC));

  if (cidString.size() < 2 || cidString[0] != base58BtcPrefix)
    return false;

  std::vector<uint8_t> bytes;
  if (!CRYPTO::CBase58BTC::DecodeBase58(cidString.substr(1), bytes))
    return false;

  CCID decoded(bytes);
  if (decoded.Empty() || decoded.Multihash().empty())
    return false;

  cid = std::move(decoded);
  return true;
}

void CCID::Deserialize(const std::vector<uint8_t>& data)
{
  SetOwnedBytes(data);
}

void CCID::Deserialize(const uint8_t* data, size_t size)
{
  if (data == nullptr)
  {
    if (size != 0)
    {
      CLog::Log(LOGERROR, "CID: Cannot deserialize {} bytes from a null data pointer", size);
      return;
    }

    SetOwnedBytes({});
    return;
  }

  SetOwnedBytes(std::vector<uint8_t>(data, data + size));
}

void CCID::SetOwnedBytes(std::vector<uint8_t> bytes)
{
  m_storageMode = StorageMode::Owning;
  m_bytes = std::move(bytes);
  m_viewData = nullptr;
  m_viewSize = 0;
  ParseBytes();
}

void CCID::SetViewBytes(const uint8_t* data, size_t size)
{
  if (data == nullptr && size != 0)
  {
    CLog::Log(LOGERROR, "CID: Cannot view {} bytes from a null data pointer", size);
    return;
  }

  m_storageMode = StorageMode::View;
  m_bytes.clear();
  m_viewData = data;
  m_viewSize = size;
  ParseBytes();
}

void CCID::ParseBytes()
{
  m_codec = CIDCodec::RAW;
  m_multihash.clear();

  const uint8_t* data = Data();
  const size_t size = Size();
  if (data == nullptr || size == 0)
    return;

  const auto decodeWithConsumedBytes =
      [](const uint8_t* source, size_t sourceSize, uint64_t& value, size_t& consumed)
  {
    value = 0;
    consumed = 0;

    if (source == nullptr || sourceSize == 0)
      return false;

    if (!CRYPTO::VarInt::Decode(source, sourceSize, value))
      return false;

    for (; consumed < sourceSize; ++consumed)
    {
      if ((source[consumed] & 0x80) == 0)
      {
        ++consumed;
        return true;
      }
    }

    return false;
  };

  uint64_t version = 0;
  size_t versionSize = 0;
  if (!decodeWithConsumedBytes(data, size, version, versionSize) || version != CID_VERSION)
    return;

  CRYPTO::CMulticodec::Code codec = CRYPTO::CMulticodec::Code::UNKNOWN;
  size_t codecSize = 0;
  if (!CRYPTO::CMulticodec::Decode(data + versionSize, size - versionSize, codec, codecSize))
    return;

  m_codec = codec;

  const size_t multihashOffset = versionSize + codecSize;
  m_multihash.assign(data + multihashOffset, data + size);
}

std::vector<uint8_t> CCID::SerializeCID(CIDCodec codec, const std::vector<uint8_t>& multihash)
{
  const uint64_t codecValue = static_cast<uint64_t>(codec);
  const size_t versionLength = CRYPTO::VarInt::EncodingLength(CID_VERSION);
  std::vector<uint8_t> encodedCodec;
  if (!CRYPTO::CMulticodec::Encode(codec, encodedCodec))
  {
    CLog::Log(LOGERROR, "CID: Failed to encode multicodec {}", codecValue);
    return {};
  }

  const size_t codecLength = encodedCodec.size();

  std::vector<uint8_t> bytes(versionLength + codecLength + multihash.size());

  if (!CRYPTO::VarInt::EncodeTo(CID_VERSION, bytes.data(), bytes.size()))
  {
    CLog::Log(LOGERROR, "CID: Failed to encode CID version");
    return {};
  }

  std::copy(encodedCodec.begin(), encodedCodec.end(), bytes.begin() + versionLength);
  std::copy(multihash.begin(), multihash.end(), bytes.begin() + versionLength + codecLength);

  return bytes;
}
