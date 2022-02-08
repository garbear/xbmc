/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Multihash.h"

#include "utils/Digest.h"
#include "utils/log.h"

#include <limits>
#include <utility>

using namespace KODI;
using namespace CRYPTO;

namespace
{
UTILITY::CDigest::Type DigestTypeForIdentifier(CMultihash::Identifier identifier)
{
  switch (identifier)
  {
    case CMultihash::Identifier::SHA1:
      return UTILITY::CDigest::Type::SHA1;
    case CMultihash::Identifier::SHA2_256:
      return UTILITY::CDigest::Type::SHA256;
    case CMultihash::Identifier::SHA2_512:
      return UTILITY::CDigest::Type::SHA512;
    case CMultihash::Identifier::MD5:
      return UTILITY::CDigest::Type::MD5;
    default:
      break;
  }

  return UTILITY::CDigest::Type::INVALID;
}
} // namespace

bool CMultihash::TranslateIdentifier(Identifier identifierEnum, uint16_t& identifier)
{
  bool success = true;

  switch (identifierEnum)
  {
    case Identifier::IDENTITY:
    {
      identifier = 0x00;
      break;
    }
    case Identifier::SHA1:
    {
      identifier = 0x11;
      break;
    }
    case Identifier::SHA2_256:
    {
      identifier = 0x12;
      break;
    }
    case Identifier::SHA2_512:
    {
      identifier = 0x13;
      break;
    }
    case Identifier::SHA3_512:
    {
      identifier = 0x14;
      break;
    }
    case Identifier::SHA3_384:
    {
      identifier = 0x15;
      break;
    }
    case Identifier::SHA3_256:
    {
      identifier = 0x16;
      break;
    }
    case Identifier::SHA3_224:
    {
      identifier = 0x17;
      break;
    }
    case Identifier::KECCAK_224:
    {
      identifier = 0x1a;
      break;
    }
    case Identifier::KECCAK_256:
    {
      identifier = 0x1b;
      break;
    }
    case Identifier::KECCAK_384:
    {
      identifier = 0x1c;
      break;
    }
    case Identifier::KECCAK_512:
    {
      identifier = 0x1d;
      break;
    }
    case Identifier::MD5:
    {
      identifier = 0xd5;
      break;
    }
    default:
    {
      success = false;
      break;
    }
  }

  return success;
}

CMultihash::Identifier CMultihash::TranslateIdentifier(uint16_t identifier)
{
  switch (identifier)
  {
    case 0x00:
      return Identifier::IDENTITY;
    case 0x11:
      return Identifier::SHA1;
    case 0x12:
      return Identifier::SHA2_256;
    case 0x13:
      return Identifier::SHA2_512;
    case 0x14:
      return Identifier::SHA3_512;
    case 0x15:
      return Identifier::SHA3_384;
    case 0x16:
      return Identifier::SHA3_256;
    case 0x17:
      return Identifier::SHA3_224;
    case 0x1a:
      return Identifier::KECCAK_224;
    case 0x1b:
      return Identifier::KECCAK_256;
    case 0x1c:
      return Identifier::KECCAK_384;
    case 0x1d:
      return Identifier::KECCAK_512;
    case 0xd5:
      return Identifier::MD5;
    default:
      break;
  }

  return Identifier::UNKNOWN;
}

std::string CMultihash::ToString(Identifier identifier)
{
  switch (identifier)
  {
    case Identifier::IDENTITY:
      return "identity";
    case Identifier::SHA1:
      return "sha1";
    case Identifier::SHA2_256:
      return "sha2-256";
    case Identifier::SHA2_512:
      return "sha2-512";
    case Identifier::SHA3_512:
      return "sha3-512";
    case Identifier::SHA3_384:
      return "sha3-384";
    case Identifier::SHA3_256:
      return "sha3-256";
    case Identifier::SHA3_224:
      return "sha3-224";
    case Identifier::KECCAK_224:
      return "keccak-224";
    case Identifier::KECCAK_256:
      return "keccak-256";
    case Identifier::KECCAK_384:
      return "keccak-384";
    case Identifier::KECCAK_512:
      return "keccak-512";
    case Identifier::MD5:
      return "md5";
    default:
      break;
  }

  return "unknown";
}

CMultihash::CMultihash(Identifier identifier, std::vector<uint8_t> data)
  : m_identifier(identifier),
    m_digest(std::move(data)),
    m_finalized(!m_digest.empty())
{
}

CMultihash::CMultihash() = default;

CMultihash::~CMultihash() = default;

CMultihash::CMultihash(const std::vector<uint8_t>& multihash)
{
  Deserialize(multihash);
}

void CMultihash::SetIdentifier(Identifier identifier)
{
  m_identifier = identifier;
  m_hasher.reset();
  m_finalized = false;
  m_invalid = false;
}

void CMultihash::SetData(std::vector<uint8_t> data)
{
  m_digest = std::move(data);
  m_hasher.reset();
  m_finalized = !m_digest.empty();
  m_invalid = false;
}

bool CMultihash::EnsureHasher()
{
  if (m_identifier == Identifier::IDENTITY)
    return true;

  if (m_invalid)
    return false;

  if (m_hasher)
    return true;

  const UTILITY::CDigest::Type digestType = DigestTypeForIdentifier(m_identifier);
  if (digestType == UTILITY::CDigest::Type::INVALID)
  {
    CLog::Log(LOGERROR, "CMultihash::EnsureHasher - unsupported digest identifier {}",
              ToString(m_identifier));
    m_invalid = true;
    return false;
  }

  m_hasher = std::make_unique<UTILITY::CDigest>(digestType);
  return true;
}

void CMultihash::Update(void const* data, std::size_t size)
{
  if (m_finalized)
    return;

  if (m_identifier == Identifier::IDENTITY)
  {
    if (data != nullptr && size != 0)
    {
      const auto* bytes = static_cast<const uint8_t*>(data);
      m_digest.insert(m_digest.end(), bytes, bytes + size);
    }
    return;
  }

  if (!EnsureHasher())
    return;

  if (data != nullptr && size != 0)
    m_hasher->Update(data, size);
}

void CMultihash::Finalize()
{
  if (m_finalized)
    return;

  if (m_identifier == Identifier::IDENTITY)
  {
    m_finalized = true;
    return;
  }

  if (!EnsureHasher())
    return;

  const std::string digest = m_hasher->FinalizeRaw();
  m_digest.assign(digest.begin(), digest.end());
  m_hasher.reset();
  m_finalized = true;
}

bool CMultihash::Serialize(std::vector<uint8_t>& multihash) const
{
  if (m_invalid)
    return false;

  uint16_t identifier;
  if (!TranslateIdentifier(m_identifier, identifier))
    return false;

  if (identifier > std::numeric_limits<uint8_t>::max())
  {
    CLog::Log(LOGERROR,
              "CMultihash::Serialize - identifier {} cannot be serialized in the "
              "temporary single-byte multihash format",
              identifier);
    return false;
  }

  if (m_digest.size() > std::numeric_limits<uint8_t>::max())
  {
    CLog::Log(LOGERROR,
              "CMultihash::Serialize - digest size {} cannot be serialized in the "
              "temporary single-byte multihash format",
              m_digest.size());
    return false;
  }

  std::vector<uint8_t> serialized;
  serialized.reserve(m_digest.size() + 2);
  serialized.push_back(static_cast<uint8_t>(identifier));
  serialized.push_back(static_cast<uint8_t>(m_digest.size()));
  serialized.insert(serialized.end(), m_digest.begin(), m_digest.end());

  multihash = std::move(serialized);
  return true;
}

bool CMultihash::Deserialize(const std::vector<uint8_t>& multihash)
{
  Identifier identifier = Identifier::UNKNOWN;
  std::vector<uint8_t> digest;

  if (multihash.empty())
  {
    identifier = Identifier::IDENTITY;
  }
  else if (multihash.size() >= 2)
  {
    //! @todo Use VarInt once the helper reports how many bytes were consumed.
    identifier = TranslateIdentifier(static_cast<uint16_t>(multihash[0]));
    if (identifier != Identifier::UNKNOWN)
    {
      const uint8_t length = multihash[1];
      if (multihash.size() - 2 == length)
        digest.assign(multihash.begin() + 2, multihash.end());
      else
        identifier = Identifier::UNKNOWN;
    }
  }

  if (identifier == Identifier::UNKNOWN)
    return false;

  m_identifier = identifier;
  m_digest = std::move(digest);
  m_hasher.reset();
  m_finalized = !m_digest.empty();
  m_invalid = false;
  return true;
}
