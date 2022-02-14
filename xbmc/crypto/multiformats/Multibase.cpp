/*
 *  Copyright (C) 2022 Team Kodi
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

CMultibase::CMultibase(Encoding encoding, std::vector<uint8_t> data)
  : m_encoding(encoding), m_data(std::move(data))
{
}

CMultibase::CMultibase(const std::vector<uint8_t>& multibase)
{
  Deserialize(multibase);
}

CMultibase::CMultibase(const std::string& multibase)
{
  Deserialize(multibase);
}

bool CMultibase::Serialize(std::vector<uint8_t>& multibase) const
{
  bool success = true;

  if (m_encoding != Encoding::UNKNOWN)
    multibase.emplace_back(TranslateEncoding(m_encoding));

  switch (m_encoding)
  {
    case Encoding::IDENTITY:
    {
      multibase.insert(multibase.end(), m_data.begin(), m_data.end());
      break;
    }
    case Encoding::BASE58BTC:
    {
      //! @todo Encode in-place to avoid temporary string
      std::string encoded = CBase58BTC::EncodeBase58(m_data);
      multibase.insert(multibase.end(), encoded.begin(), encoded.end());
      break;
    }
    default:
      success = false;
  }

  return success;
}

bool CMultibase::Serialize(std::string& multibase) const
{
  bool success = true;

  if (m_encoding != Encoding::UNKNOWN)
    multibase.push_back(TranslateEncoding(m_encoding));

  switch (m_encoding)
  {
    case Encoding::IDENTITY:
    {
      multibase.insert(multibase.end(), m_data.begin(), m_data.end());
      break;
    }
    case Encoding::BASE58BTC:
    {
      std::string encoded = CBase58BTC::EncodeBase58(m_data);
      multibase.insert(multibase.end(), encoded.begin(), encoded.end());
      break;
    }
    default:
      success = false;
  }

  return success;
}

bool CMultibase::Deserialize(const std::vector<uint8_t>& multibase)
{
  bool success = false;

  if (multibase.empty())
  {
    m_encoding = Encoding::UNKNOWN;
    m_data.clear();
    success = true;
  }
  else
  {
    Encoding encoding = TranslateEncoding(multibase[0]);
    switch (encoding)
    {
      case Encoding::IDENTITY:
      {
        m_data.assign(multibase.begin() + 1, multibase.end());
        success = true;
        break;
      }
      case Encoding::BASE58BTC:
      {
        std::string base58(multibase.begin() + 1, multibase.end());
        success = CBase58BTC::DecodeBase58(base58, m_data);
        break;
      }
      default:
        break;
    }

    if (success)
      m_encoding = encoding;
  }

  return success;
}

bool CMultibase::Deserialize(const std::string& multibase)
{
  bool success = false;

  if (multibase.empty())
  {
    m_encoding = Encoding::UNKNOWN;
    m_data.clear();
    success = true;
  }
  else
  {
    Encoding encoding = TranslateEncoding(static_cast<uint8_t>(multibase[0]));
    switch (encoding)
    {
      case Encoding::IDENTITY:
      {
        m_data.assign(multibase.begin() + 1, multibase.end());
        success = true;
        break;
      }
      case Encoding::BASE58BTC:
      {
        std::string base58(multibase.substr(1));
        success = CBase58BTC::DecodeBase58(base58, m_data);
        break;
      }
      default:
        break;
    }

    if (success)
      m_encoding = encoding;
  }

  return success;
}
