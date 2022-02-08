/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "crypto/CryptoTypes.h"

#include <stdint.h>
#include <string>

namespace KODI
{
namespace CRYPTO
{
/*!
 * \brief Encode/decode to/from base58 format
 *
 * Implementation is taken from
 * https://github.com/bitcoin/bitcoin/blob/master/src/base58.h
 */
class CBase58BTC
{
public:
  /*!
   * \brief Encode bytes to base58 string
   *
   * \param data Pointer to the data to be encoded
   * \param dataSize Size of the data to be encoded
   *
   * \return Encoded string
   */
  static std::string EncodeBase58(const uint8_t* data, size_t dataSize);

  /*!
   * \brief Decode base58 string to a byte vector
   *
   * \param base58String String to be decoded
   * \param[out] data Output buffer for the decoded bytes in case of success,
   *             not used otherwise
   *
   * \return True in case of success, false otherwise
   */
  static bool DecodeBase58(const std::string& base58String, std::vector<uint8_t>& data);
};
} // namespace CRYPTO
} // namespace KODI
