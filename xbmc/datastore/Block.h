/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CID.h"

#include <stdint.h>
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
   * \brief Get the CID
   */
  const CCID& CID() const { return m_cid; }

  /*!
   * \brief Set the CID
   */
  void SetCID(CCID cid);

  /*!
   * \brief Get the data (const)
   */
  const uint8_t* Data() const { return m_data.get(); }

  /*!
   * \brief Get the data (mutable)
   */
  uint8_t* Data() { return m_data.get(); }

  /*!
   * \brief Set the data
   */
  void SetData(std::vector<uint8_t> data) { m_data = std::move(data); }

  /*!
   * \brief Serialize as an array of bytes
   */
  std::vector<uint8_t> Serialize() const;

  /*!
   * \brief Deserialize an array of bytes
   */
  void Deserialize(const std::vector<uint8_t>& data);

private:
  // Block parameters
  CCID m_cid;
  std::vector<uint8_t> m_data;
};
} // namespace DATASTORE
} // namespace KODI
