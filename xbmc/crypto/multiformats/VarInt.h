/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace KODI
{
namespace CRYPTO
{

class VarInt
{
public:
  /*!
   * \brief Decode a varint from a byte array
   *
   * \param data Pointer to the byte array to decode
   * \param dataLength Length of the byte array
   * \param[out] result The decoded integer
   *
   * \return True if the integer was decoded successfully, false otherwise
   */
  static bool Decode(const uint8_t* data, size_t dataLength, uint64_t& result);

  /*!
   * \brief Encode a varint to a target byte array at the specified offset
   *
   * \param value The integer to encode
   * \param target Pointer to the byte array to encode to
   * \param targetLength Length of the target byte array
   *
   * \return True if the integer was encoded successfully, false otherwise
   */
  static bool EncodeTo(uint64_t value, uint8_t* target, size_t targetLength);

  /*!
   * \brief Returns the number of bytes needed to encode the given integer as
   * a varint
   *
   * \param value The integer to encode
   *
   * \return The number of bytes needed to encode the given integer as a varint
   */
  static size_t EncodingLength(uint64_t value);
};

} // namespace CRYPTO
} // namespace KODI
