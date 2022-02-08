/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Multihash.h"

#include "utils/Digest.h"

using namespace KODI;
using namespace CRYPTO;

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
  : m_identifier(identifier), m_digest(std::move(data))
{
}

CMultihash::CMultihash(const std::vector<uint8_t>& multihash)
{
  Deserialize(multihash);
}

bool CMultihash::Serialize(std::vector<uint8_t>& multihash) const
{
  bool success = false;

  uint16_t identifier;
  if (TranslateIdentifier(m_identifier, identifier))
  {
    multihash.push_back(identifier);
    multihash.push_back(static_cast<uint8_t>(m_digest.size()));
    multihash.insert(multihash.end(), m_digest.begin(), m_digest.end());
    success = true;
  }

  return success;
}

bool CMultihash::Deserialize(const std::vector<uint8_t>& multihash)
{
  bool success = false;

  if (multihash.empty())
  {
    m_identifier = Identifier::IDENTITY;
    m_digest.clear();
    success = true;
  }
  else if (multihash.size() >= 2)
  {
    //! @todo Varint
    Identifier identifier = TranslateIdentifier(static_cast<uint16_t>(multihash[0]));
    if (identifier != Identifier::UNKNOWN)
    {
      const uint8_t length = multihash[1];
      if (length + 2 == static_cast<int>(multihash.size()))
      {
        m_digest.assign(multihash.begin() + 2, multihash.end());
        success = true;
      }
    }
  }

  return success;
}
