/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <utility>
#include <vector>

namespace KODI
{
namespace DATASTORE
{
  /*!
   * \brief Hash algorithms used by multihash
   *
   * Matches the codes described in the spec:
   *
   * https://github.com/multiformats/multihash/blob/master/hashtable.csv
   */
  enum class MultihashEncoding
  {
    UNKNOWN = 0x00,
    MD5 = 0xd5,
    SHA1 = 0x11,
    SHA2_256 = 0x12,
    SHA2_512 = 0x13,
    SHA3_224 = 0x17,
    SHA3_256 = 0x16,
    SHA3_384 = 0x15,
    SHA3_512 = 0x14,
  };

  /*!
   * \brief Multihash is a protocol for combining size and encoding
   *        considerations
   */
  class CMultihash
  {
  public:
    /*!
     * \brief Construct an empty multihash
     */
    CMultihash() = default;

    /*!
     * \brief Construct a multihash from parameters
     *
     * \param encoding The hash algorithm used to encode data
     * \param digest The hash digest
     */
    CMultihash(MultihashEncoding encoding, std::vector<uint8_t> digest);

    /*!
     * \brief Copy-construct a multihash
     *
     * \param multihash The multihash being copied
     */
    CMultihash(const CMultihash& multihash) = default;

    /*!
     * \brief Copy-assign a multihash
     *
     * \param multihash The multihash being copied
     */
    CMultihash &operator=(const CMultihash& multihash) = default;

    /*!
     * \brief Move-construct a multihash
     *
     * Makes a copy of the object representation as if by std::memmove().
     *
     * \param multihash The multihash being moved
     */
    CMultihash(CMultihash&& multihash) = default;

    /*!
     * \brief Move-assign a multihash
     *
     * Makes a copy of the object representation as if by std::memmove().
     *
     * \param multihash The multihash being moved
     */
    CMultihash& operator=(CMultihash&& multihash) = default;

    /*!
     * \brief Get the multihash encoding
     */
    MultihashEncoding Encoding() const { return m_encoding; }

    /*!
     * \brief Get the multihash digest
     */
    const std::vector<uint8_t>& Digest() const { return m_digest; }

    /*!
     * \brief Set the multihash properties
     */
    void SetProperties(MultihashEncoding encoding, std::vector<uint8_t> digest);

    /*!
     * \brief Serialize as an array of bytes
     */
    std::vector<uint8_t> Serialize() const;

    /*!
     * \brief Deserialize an array of bytes
     */
    void Deserialize(const std::vector<uint8_t>& data);

  private:
    // Multihash parameters
    MultihashEncoding m_encoding = MultihashEncoding::UNKNOWN;
    std::vector<uint8_t> m_digest;
  };
}
}
