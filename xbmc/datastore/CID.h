/*
 *  Copyright (C) 2018-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "crypto/multiformats/Multicodec.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

namespace KODI
{
namespace DATASTORE
{
/*!
 * \brief Multicodec packed content type used by CIDs
 */
using CIDCodec = KODI::CRYPTO::CMulticodec::Code;

/*!
   * \brief Implementation of the Content ID v1 spec for content-addressed
   *        identifiers
   */
class CCID
{
public:
  /*!
     * \brief Construct an empty CID
     */
  CCID() = default;

  /*!
     * \brief Copy-construct a CID
     *
     * \param cid The CID being copied
     */
  CCID(const CCID& cid);

  /*!
     * \brief Move-construct a CID
     *
     * Preserves the current ownership/view mode. Moving a view-backed CID does
     * not extend the lifetime of the external buffer; callers must keep the
     * external storage alive while the moved-to CID views it.
     *
     * \param cid The CID being moved
     */
  CCID(CCID&& cid) = default;

  /*!
     * \brief Construct a CID from parameters
     *
     * \param codec The CID codec
     * \param multihash The multihash
     */
  CCID(CIDCodec codec, std::vector<uint8_t> multihash);

  /*!
     * \brief Construct a CID from owned serialized bytes
     *
     * \param bytes The serialized CID bytes
     */
  explicit CCID(std::vector<uint8_t> bytes);

  /*!
     * \brief Copy serialized CID bytes into owned storage
     *
     * \param data The serialized CID bytes
     * \param size The size of the serialized CID bytes
     *
     * \return A CID owning a copy of the bytes, or an empty/default CID if
     *         data is nullptr and size is nonzero
     */
  static CCID FromBytes(const uint8_t* data, size_t size);

  /*!
     * \brief Refer to external serialized CID bytes
     *
     * The caller must keep the referenced bytes alive for the lifetime of the
     * CID object or until the CID object is assigned new storage.
     *
     * \param data The serialized CID bytes
     * \param size The size of the serialized CID bytes
     *
     * \return A CID viewing external bytes, or an empty/default CID if data is
     *         nullptr and size is nonzero
     */
  static CCID ViewBytes(const uint8_t* data, size_t size);

  /*!
     * \brief Copy-assign a CID
     *
     * \param cid The CID being copied
     */
  CCID& operator=(const CCID& cid);

  /*!
     * \brief Move-assign a CID
     *
     * Preserves the current ownership/view mode. Moving a view-backed CID does
     * not extend the lifetime of the external buffer; callers must keep the
     * external storage alive while the moved-to CID views it.
     *
     * \param cid The CID being moved
     */
  CCID& operator=(CCID&& cid) = default;

  bool operator==(const CCID& other) const;

  /*!
     * \brief Get a pointer to serialized CID bytes
     */
  const uint8_t* Data() const;

  /*!
     * \brief Get the serialized CID byte count
     */
  size_t Size() const;

  /*!
     * \brief Return true if the CID has no serialized bytes
     */
  bool Empty() const { return Size() == 0; }

  /*!
     * \brief Return true if the CID owns its serialized bytes
     */
  bool IsOwning() const;

  /*!
     * \brief Get the CID codec
     */
  CIDCodec Codec() const { return m_codec; }

  /*!
     * \brief Set the CID codec
     */
  void SetCodec(CIDCodec codec);

  /*!
     * \brief Get the multihash
     */
  const std::vector<uint8_t>& Multihash() const { return m_multihash; }

  /*!
     * \brief Set the multihash
     */
  void SetMultihash(std::vector<uint8_t> multihash);

  /*!
     * \brief Serialize as an array of bytes
     */
  std::pair<const uint8_t*, size_t> Serialize() const;

  /*!
     * \brief Encode as a multibase base58btc CID string
     */
  std::string ToString() const;

  /*!
     * \brief Decode a multibase base58btc CID string
     *
     * \param cidString The encoded CID string
     * \param cid The decoded CID, left unchanged on failure
     *
     * \return True on success, false on invalid input
     */
  static bool FromString(const std::string& cidString, CCID& cid);

  /*!
     * \brief Deserialize an array of bytes
     */
  void Deserialize(const std::vector<uint8_t>& data);

  /*!
     * \brief Deserialize an array of bytes
     *
     * If data is nullptr and size is nonzero, the CID is left unchanged.
     */
  void Deserialize(const uint8_t* data, size_t size);

private:
  enum class StorageMode
  {
    Owning,
    View,
  };

  StorageMode m_storageMode = StorageMode::Owning;
  std::vector<uint8_t> m_bytes;
  const uint8_t* m_viewData = nullptr;
  size_t m_viewSize = 0;

  // Cached CID parameters parsed from serialized bytes
  CIDCodec m_codec = CIDCodec::RAW;
  std::vector<uint8_t> m_multihash;

  void SetOwnedBytes(std::vector<uint8_t> bytes);
  void SetViewBytes(const uint8_t* data, size_t size);

  void ParseBytes();
  static std::vector<uint8_t> SerializeCID(CIDCodec codec, const std::vector<uint8_t>& multihash);
};
} // namespace DATASTORE
} // namespace KODI
