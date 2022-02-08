/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
namespace UTILITY
{
class CDigest;
}

namespace CRYPTO
{
/*!
 * \brief Implementation of the Multihash protocol for self-identifying hashes
 *
 * Multihash is a protocol for differentiating outputs from various
 * well-established cryptographic hash functions, addressing size + identifier
 * considerations.
 *
 * The latest specification can be found at:
 *
 *   https://github.com/multiformats/multihash
 *
 * Multibase is in IETF draft phase. The latest draft can be found at:
 *
 *   https://datatracker.ietf.org/doc/draft-multiformats-multihash
 */
class CMultihash
{
public:
  /*!
   * \brief Multihash Identifier Registry
   */
  enum class Identifier
  {
    UNKNOWN,
    IDENTITY,
    SHA1, // RFC 6234
    SHA2_256, // RFC 6234
    SHA2_512, // RFC 6234
    SHA3_512,
    SHA3_384,
    SHA3_256,
    SHA3_224,
    KECCAK_224,
    KECCAK_256,
    KECCAK_384,
    KECCAK_512,
    MD5, // RFC 6151 (deprecated)
  };

  static bool TranslateIdentifier(Identifier identifierEnum, uint16_t& identifier);
  static Identifier TranslateIdentifier(uint16_t identifier);

  /*!
   * brief Convert the multihash identifier to a string suitable for logging
   */
  static std::string ToString(Identifier identifier);

  CMultihash() = default;
  explicit CMultihash(Identifier identifier, std::vector<uint8_t> data);
  explicit CMultihash(const std::vector<uint8_t>& multihash);

  Identifier GetIdentifier() const { return m_identifier; }
  const std::vector<uint8_t>& GetData() const { return m_digest; }

  void SetIdentifier(Identifier identifier) { m_identifier = identifier; }
  void SetData(std::vector<uint8_t> data) { m_digest = std::move(data); }

  /*!
   * Update digest with data
   *
   * Cannot be called after \ref Finalize has been called
   */
  void Update(void const* data, std::size_t size) {}

  /**
   * Finalize the digest
   */
  void Finalize() {}

  bool Serialize(std::vector<uint8_t>& multihash) const;
  bool Deserialize(const std::vector<uint8_t>& multihash);

private:
  Identifier m_identifier = Identifier::IDENTITY;
  std::vector<uint8_t> m_digest;
  std::unique_ptr<UTILITY::CDigest> m_hasher;
};

} // namespace CRYPTO
} // namespace KODI
