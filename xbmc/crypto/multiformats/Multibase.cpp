/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Multibase.h"

using namespace KODI;
using namespace CRYPTO;

#include "crypto/codecs/Base58BTC.h"

uint8_t CMultibase::TranslateEncoding(Encoding encodingEnum)
{
  switch (encodingEnum)
  {
    case Encoding::IDENTITY:
      return '\0';
    case Encoding::BASE2:
      return '0';
    case Encoding::BASE8:
      return '7';
    case Encoding::BASE10:
      return '9';
    case Encoding::BASE16:
      return 'f';
    case Encoding::BASE16UPPER:
      return 'F';
    case Encoding::BASE32HEX:
      return 'v';
    case Encoding::BASE32HEXUPPER:
      return 'V';
    case Encoding::BASE32HEXPAD:
      return 't';
    case Encoding::BASE32HEXPADUPPER:
      return 'T';
    case Encoding::BASE32:
      return 'b';
    case Encoding::BASE32UPPER:
      return 'B';
    case Encoding::BASE32PAD:
      return 'c';
    case Encoding::BASE32PADUPPER:
      return 'C';
    case Encoding::BASE32Z:
      return 'h';
    case Encoding::BASE36:
      return 'k';
    case Encoding::BASE36UPPER:
      return 'K';
    case Encoding::BASE58BTC:
      return 'z';
    case Encoding::BASE58FLICKR:
      return 'Z';
    case Encoding::BASE64:
      return 'm';
    case Encoding::BASE64PAD:
      return 'M';
    case Encoding::BASE64URL:
      return 'u';
    case Encoding::BASE64URLPAD:
      return 'U';
    case Encoding::PROQUINT:
      return 'p';
    default:
      break;
  }

  return '?';
}

CMultibase::Encoding CMultibase::TranslateEncoding(uint8_t encodingChar)
{
  switch (encodingChar)
  {
    case '\0':
      return Encoding::IDENTITY;
    case '0':
      return Encoding::BASE2;
    case '7':
      return Encoding::BASE8;
    case '9':
      return Encoding::BASE10;
    case 'f':
      return Encoding::BASE16;
    case 'F':
      return Encoding::BASE16UPPER;
    case 'v':
      return Encoding::BASE32HEX;
    case 'V':
      return Encoding::BASE32HEXUPPER;
    case 't':
      return Encoding::BASE32HEXPAD;
    case 'T':
      return Encoding::BASE32HEXPADUPPER;
    case 'b':
      return Encoding::BASE32;
    case 'B':
      return Encoding::BASE32UPPER;
    case 'c':
      return Encoding::BASE32PAD;
    case 'C':
      return Encoding::BASE32PADUPPER;
    case 'h':
      return Encoding::BASE32Z;
    case 'k':
      return Encoding::BASE36;
    case 'K':
      return Encoding::BASE36UPPER;
    case 'z':
      return Encoding::BASE58BTC;
    case 'Z':
      return Encoding::BASE58FLICKR;
    case 'm':
      return Encoding::BASE64;
    case 'M':
      return Encoding::BASE64PAD;
    case 'u':
      return Encoding::BASE64URL;
    case 'U':
      return Encoding::BASE64URLPAD;
    case 'p':
      return Encoding::PROQUINT;
    default:
      break;
  }

  return Encoding::UNKNOWN;
}

std::string CMultibase::ToString(Encoding encoding)
{
  switch (encoding)
  {
    case Encoding::IDENTITY:
      return "identity";
    case Encoding::BASE2:
      return "base2";
    case Encoding::BASE8:
      return "base8";
    case Encoding::BASE10:
      return "base10";
    case Encoding::BASE16:
      return "base16";
    case Encoding::BASE16UPPER:
      return "base16upper";
    case Encoding::BASE32HEX:
      return "base32hex";
    case Encoding::BASE32HEXUPPER:
      return "base32hexupper";
    case Encoding::BASE32HEXPAD:
      return "base32hexpad";
    case Encoding::BASE32HEXPADUPPER:
      return "base32hexpadupper";
    case Encoding::BASE32:
      return "base32";
    case Encoding::BASE32UPPER:
      return "base32upper";
    case Encoding::BASE32PAD:
      return "base32pad";
    case Encoding::BASE32PADUPPER:
      return "base32padupper";
    case Encoding::BASE32Z:
      return "base32z";
    case Encoding::BASE36:
      return "base36";
    case Encoding::BASE36UPPER:
      return "base36upper";
    case Encoding::BASE58BTC:
      return "base58btc";
    case Encoding::BASE58FLICKR:
      return "base58flickr";
    case Encoding::BASE64:
      return "base64";
    case Encoding::BASE64PAD:
      return "base64pad";
    case Encoding::BASE64URL:
      return "base64url";
    case Encoding::BASE64URLPAD:
      return "base64urlpad";
    case Encoding::PROQUINT:
      return "proquint";
    default:
      break;
  }

  return "unknown";
}

CMultibase::CMultibase(Encoding encoding, const uint8_t* payload, size_t payloadSize)
{
  SetData(encoding, payload, payloadSize);
}

CMultibase::CMultibase(const std::string& multibase)
{
  SetData(multibase);
}

CMultibase::CMultibase(const uint8_t* multibaseData, size_t multibaseSize)
{
  SetData(multibaseData, multibaseSize);
}

CMultibase::Encoding CMultibase::GetEncoding() const
{
  if (m_dataPtr != nullptr && m_dataSize > 0)
    return TranslateEncoding(m_dataPtr[0]);

  return Encoding::UNKNOWN;
}

std::pair<const uint8_t*, size_t> CMultibase::GetData() const
{
  return std::make_pair(m_dataPtr, m_dataSize);
}

void CMultibase::SetData(Encoding encoding, const uint8_t* data, size_t size)
{
  m_multibaseBuffer.clear();

  // Reserve the space for encoding byte + payload.
  m_multibaseBuffer.reserve(size + 1);

  // Add the encoding byte.
  m_multibaseBuffer.push_back(TranslateEncoding(encoding));

  // Append the payload to the buffer.
  m_multibaseBuffer.insert(m_multibaseBuffer.end(), data, data + size);

  // Point m_dataPtr to the beginning of our buffer.
  m_dataPtr = m_multibaseBuffer.data();

  // Set the data size.
  m_dataSize = m_multibaseBuffer.size();
}

void CMultibase::SetData(const std::string& multibase)
{
  if (!multibase.empty())
  {
    m_multibaseBuffer.assign(multibase.begin(), multibase.end());
    m_dataPtr = m_multibaseBuffer.data();
    m_dataSize = m_multibaseBuffer.size();
  }
  else
  {
    m_multibaseBuffer.clear();
    m_dataPtr = nullptr;
    m_dataSize = 0;
  }
}

void CMultibase::SetData(const uint8_t* multibaseData, size_t multibaseSize)
{
  if (multibaseData != nullptr && multibaseSize > 0)
  {
    // Directly use the pointer to the externally owned data
    m_dataPtr = multibaseData;

    // Set the data size
    m_dataSize = multibaseSize;
  }
  else
  {
    m_dataPtr = nullptr;
    m_dataSize = 0;
  }
}

std::string CMultibase::Serialize() const
{
  return std::string(m_multibaseBuffer.begin(), m_multibaseBuffer.end());
}
