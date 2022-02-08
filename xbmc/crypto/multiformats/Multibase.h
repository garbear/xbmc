/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <utility>
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

  /*!
   * \brief Default constructor.
   */
  CMultibase() = default;

  /*!
   * \brief Constructor taking encoding enum and a pointer to data payload
   */
  explicit CMultibase(Encoding encoding, const uint8_t* payload, size_t payloadSize);

  /*!
   * \brief Constructor taking a multibase encoded string
   */
  explicit CMultibase(const std::string& multibase);

  /*!
   * \brief Initializes a CMultibase object using raw byte data.
   *
   * This constructor creates a CMultibase object using the given raw byte
   * data as input. The provided data is expected to be in the multibase
   * format, with  the first byte indicating the encoding type followed by
   * the actual encoded data.
   *
   * The object does not take ownership of the data, so the caller is
   * responsible for ensuring the data remains valid for the lifetime of the
   * CMultibase object.
   *
   * \param[in] multibaseData Pointer to the raw byte data.
   * \param[in] multibaseSize Size of the provided data.
   */
  explicit CMultibase(const uint8_t* multibaseData, size_t multibaseSize);

  /*!
   * \brief Get the encoding from the internal buffer.
   */
  Encoding GetEncoding() const;

  /*!
   * \brief Get a pointer to the raw data (includes encoding byte).
   */
  const uint8_t* GetDataPtr() const { return m_dataPtr; }

  /*!
   * \brief Get the size of the raw data.
   */
  size_t GetDataSize() const { return m_dataSize; }

  /*!
   * \brief Get both the data pointer and its size.
   */
  std::pair<const uint8_t*, size_t> GetData() const;

  /*!
   * \brief Set the multibase encoding and raw data from a buffer
   */
  void SetData(Encoding encoding, const uint8_t* data, size_t size);

  /*!
  * \brief Set the multibase encoding and raw data from a multibase string
  */
  void SetData(const std::string& multibase);

  /**
   * @brief Set the multibase data for the current CMultibase object.
   *
   * This function sets the internal data pointer and size to the provided 
   * multibase data. The first byte of the multibase data is expected to be 
   * the encoding character. The rest of the data represents the encoded 
   * payload.
   *
   * @param[in] multibaseData Pointer to the multibase data.
   * @param[in] multibaseSize Size of the multibase data in bytes.
   */
  void SetData(const uint8_t* multibaseData, size_t multibaseSize);

  /*!
   * \brief Serialize the internal data into a multibase encoded string.
   */
  std::string Serialize() const;

private:
  // Buffer that holds both the encoding and the data
  std::vector<uint8_t> m_multibaseBuffer;

  // Pointer to the beginning of the multibase data
  const uint8_t* m_dataPtr = nullptr;

  // Size of the data (including the encoding byte)
  size_t m_dataSize = 0;
};

} // namespace CRYPTO
} // namespace KODI
