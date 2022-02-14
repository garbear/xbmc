/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
namespace CRYPTO
{
/*!
 * \brief Implementation of the Multibase protocol for self-identifying base
 *        encodings
 *
 * Multibase is a protocol for disambiguating the encoding of base-encoded
 * (e.g., base32, base36, base64, base58, etc.) binary appearing in text.
 *
 * The latest specification can be found at:
 *
 *   https://github.com/multiformats/multibase
 *
 * Multibase is in IETF draft phase. The latest draft can be found at:
 *
 *   https://www.ietf.org/id/draft-multiformats-multibase-04.html
 */
class CMultibase
{
public:
  /*!
   * \brief Multibase Table
   *
   * The current multibase table is here:
   *
   *   https://github.com/multiformats/multibase/blob/master/multibase.csv
   */
  enum class Encoding
  {
    UNKNOWN,
    IDENTITY, // 8-bit binary (encoder and decoder keeps data unmodified)
    BASE2, // binary (01010101)
    BASE8, // octal
    BASE10, // decimal
    BASE16, // hexadecimal
    BASE16UPPER, // hexadecimal
    BASE32HEX, // rfc4648 case-insensitive - no padding - highest char
    BASE32HEXUPPER, // rfc4648 case-insensitive - no padding - highest char
    BASE32HEXPAD, // rfc4648 case-insensitive - with padding
    BASE32HEXPADUPPER, // rfc4648 case-insensitive - with padding
    BASE32, // rfc4648 case-insensitive - no padding
    BASE32UPPER, // rfc4648 case-insensitive - no padding
    BASE32PAD, // rfc4648 case-insensitive - with padding
    BASE32PADUPPER, // rfc4648 case-insensitive - with padding
    BASE32Z, // z-base-32 (used by Tahoe-LAFS)
    BASE36, // base36 [0-9a-z] case-insensitive - no padding
    BASE36UPPER, // base36 [0-9a-z] case-insensitive - no padding
    BASE58BTC, // base58 bitcoin
    BASE58FLICKR, // base58 flicker
    BASE64, // rfc4648 no padding
    BASE64PAD, // rfc4648 with padding - MIME encoding
    BASE64URL, // rfc4648 no padding
    BASE64URLPAD, // rfc4648 with padding
    PROQUINT, // PRO-QUINT https://arxiv.org/html/0901.4016
  };

  static uint8_t TranslateEncoding(Encoding encodingEnum);
  static Encoding TranslateEncoding(uint8_t encodingChar);

  /*!
   * brief Convert the multibase encoding to a string suitable for logging
   */
  static std::string ToString(Encoding encoding);

  CMultibase() = default;
  explicit CMultibase(Encoding encoding, std::vector<uint8_t> data);
  explicit CMultibase(const std::vector<uint8_t>& multibase);
  explicit CMultibase(const std::string& multibase);

  Encoding GetEncoding() const { return m_encoding; }
  const std::vector<uint8_t>& GetData() const { return m_data; }

  void SetEncoding(Encoding encoding) { m_encoding = encoding; }
  void SetData(std::vector<uint8_t> data) { m_data = std::move(data); }

  bool Serialize(std::vector<uint8_t>& multibase) const;
  bool Serialize(std::string& multibase) const;

  bool Deserialize(const std::vector<uint8_t>& multibase);
  bool Deserialize(const std::string& multibase);

private:
  Encoding m_encoding = Encoding::UNKNOWN;
  std::vector<uint8_t> m_data;
};

} // namespace CRYPTO
} // namespace KODI
