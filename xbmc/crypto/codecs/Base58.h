/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "crypto/CryptoTypes.h"

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
class CBase58
{
public:
  /*!
   * \brief Encode bytes to base58 string
   *
   * \param Bytes to be encoded
   *
   * \return Encoded string
   */
  static std::string EncodeBase58(const ByteArray& bytes);

  /*!
   * \brief Decode base58 string to bytes
   *
   * \param String to be decoded
   * \param[out] bytes Decoded bytes in case of success, not used otherwise
   *
   * \return True in case of success, false otherwise
   */
  static bool DecodeBase58(const std::string& base58String, ByteArray& bytes);
};
} // namespace CRYPTO
} // namespace KODI
