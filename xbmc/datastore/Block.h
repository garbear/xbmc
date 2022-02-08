/*
 *  Copyright (C) 2018-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CID.h"

#include <stddef.h>
#include <stdint.h>
#include <utility>
#include <vector>

namespace KODI
{
namespace DATASTORE
{
/*!
 * \brief Container of raw data with a CID identifier
 */
class CBlock
{
public:
  /*!
   * \brief Construct an empty block
   */
  CBlock() = default;

  /*!
   * \brief Construct a block from parameters
   *
   * \param cid The block CID
   * \param data The block data
   */
  CBlock(CCID cid, std::vector<uint8_t> data);

  /*!
   * \brief Copy-construct a block
   *
   * \param block The block being copied
   */
  CBlock(const CBlock& block);

  /*!
   * \brief Move-construct a block
   *
   * Preserves the current ownership/view mode. Moving a view-backed block does
   * not extend the lifetime of the external buffer; callers must keep the
   * external storage alive while the moved-to block views it.
   *
   * \param block The block being moved
   */
  CBlock(CBlock&& block) = default;

  /*!
   * \brief Copy-assign a block
   *
   * \param block The block being copied
   */
  CBlock& operator=(const CBlock& block);

  /*!
   * \brief Move-assign a block
   *
   * Preserves the current ownership/view mode. Moving a view-backed block does
   * not extend the lifetime of the external buffer; callers must keep the
   * external storage alive while the moved-to block views it.
   *
   * \param block The block being moved
   */
  CBlock& operator=(CBlock&& block) = default;

  /*!
   * \brief Copy block bytes into owned storage
   *
   * \param cid The block CID
   * \param data The block data
   * \param size The block data size
   *
   * \return A block owning a copy of the bytes, or an empty/default-data block
   *         with the supplied CID if data is nullptr and size is nonzero
   */
  static CBlock FromBytes(CCID cid, const uint8_t* data, size_t size);

  /*!
   * \brief Refer to external block bytes
   *
   * The caller must keep the referenced bytes alive for the lifetime of the
   * block object or until the block object is assigned new storage.
   *
   * \param cid The block CID
   * \param data The block data
   * \param size The block data size
   *
   * \return A block viewing external bytes, or an empty/default-data block with
   *         the supplied CID if data is nullptr and size is nonzero
   */
  static CBlock ViewBytes(CCID cid, const uint8_t* data, size_t size);

  /*!
   * \brief Get the CID
   */
  const CCID& CID() const { return m_cid; }

  /*!
   * \brief Set the CID
   */
  void SetCID(CCID cid) { m_cid = std::move(cid); }

  /*!
   * \brief Get a pointer to block data
   */
  const uint8_t* Data() const;

  /*!
   * \brief Get the block data size
   */
  size_t Size() const;

  /*!
   * \brief Return true if the block has no data
   */
  bool Empty() const { return Size() == 0; }

  /*!
   * \brief Return true if the block owns its data
   */
  bool IsOwning() const;

  /*!
   * \brief Set the data to owned bytes
   */
  void SetData(std::vector<uint8_t> data);

  /*!
   * \brief Copy block bytes into owned storage
   *
   * If data is nullptr and size is nonzero, the block data is left unchanged.
   */
  void SetData(const uint8_t* data, size_t size);

  /*!
   * \brief Refer to external block bytes
   *
   * The caller must keep the referenced bytes alive for the lifetime of the
   * block object or until the block object is assigned new storage.
   *
   * If data is nullptr and size is nonzero, the block data is left unchanged.
   */
  void SetDataView(const uint8_t* data, size_t size);

  /*!
   * \brief Serialize as an array of bytes
   */
  std::pair<const uint8_t*, size_t> Serialize() const;

  /*!
   * \brief Deserialize an array of bytes
   */
  void Deserialize(const std::vector<uint8_t>& data);

  /*!
   * \brief Deserialize an array of bytes
   *
   * If data is nullptr and size is nonzero, the block data is left unchanged.
   */
  void Deserialize(const uint8_t* data, size_t size);

private:
  enum class StorageMode
  {
    Owning,
    View,
  };

  CCID m_cid;

  StorageMode m_storageMode = StorageMode::Owning;
  std::vector<uint8_t> m_data;
  const uint8_t* m_viewData = nullptr;
  size_t m_viewSize = 0;
};
} // namespace DATASTORE
} // namespace KODI
