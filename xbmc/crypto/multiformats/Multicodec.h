/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
namespace CRYPTO
{
/*!
 * \brief Implementation of Multicodec unsigned-varint prefixes
 *
 * Multicodec is the canonical table of codecs used by multiformats. Its codes
 * are usually encoded as unsigned varints and used as a prefix for the data
 * that follows.
 *
 * References:
 *
 *   https://github.com/multiformats/multicodec
 *   https://github.com/multiformats/multicodec/blob/master/table.csv
 */
class CMulticodec
{
public:
  /*!
   * \brief Multicodec table entries used by Kodi
   *
   * Official values below match the upstream table.csv.
   */
  enum class Code : uint64_t
  {
    UNKNOWN = UINT64_MAX,

    IDENTITY = 0x00,
    CIDV1 = 0x01,
    // Standard UnixFSv1-compatible codecs.
    RAW = 0x55,
    DAG_PB = 0x70,
    DAG_CBOR = 0x71,
    LIBP2P_KEY = 0x72,

    ED25519_PUB = 0xed,
    DAG_JSON = 0x0129,
    DAG_JSON_LZ4 = 0x012a,
    DAG_JSON_ZSTD = 0x012b,
    ED25519_PRIV = 0x1300,
  };

  static bool TranslateCode(Code codeEnum, uint64_t& code);
  static Code TranslateCode(uint64_t code);

  /*!
   * \brief Convert the multicodec code to a string suitable for logging
   */
  static std::string ToString(Code code);

  /*!
   * \brief Convert a multicodec string name to a known code
   */
  static Code FromString(const std::string& code);

  /*!
   * \brief Encode a multicodec code as an unsigned varint
   */
  static bool Encode(Code code, std::vector<uint8_t>& encoded);

  /*!
   * \brief Prefix payload bytes with an encoded multicodec code
   */
  static bool EncodePrefix(Code code,
                           const uint8_t* payload,
                           size_t payloadSize,
                           std::vector<uint8_t>& prefixed);

  /*!
   * \brief Decode a multicodec code from an unsigned-varint prefix
   *
   * \param data Pointer to bytes beginning with the multicodec varint
   * \param dataSize Size of the input bytes
   * \param[out] code The decoded known multicodec code
   * \param[out] bytesRead Number of bytes consumed by the varint prefix
   *
   * \return True if a known code was decoded successfully, false otherwise
   */
  static bool Decode(const uint8_t* data, size_t dataSize, Code& code, size_t& bytesRead);

  /*!
   * \brief Decode a multicodec-prefixed buffer into code and payload
   */
  static bool DecodePrefix(const uint8_t* data,
                           size_t dataSize,
                           Code& code,
                           std::vector<uint8_t>& payload);
};

} // namespace CRYPTO
} // namespace KODI
