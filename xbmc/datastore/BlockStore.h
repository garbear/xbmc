/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace DATASTORE
{
  class CBlock;
  class CCID;
  class CDataStore;

  /*!
   * \brief Thin wrapper over a data store for getting and putting block
   *        objects
   */
  class CBlockStore
  {
  public:
    /*!
     * \brief Construct a block store
     *
     * \param dataStore The underlying data store
     */
    CBlockStore(CDataStore& dataStore);

    /*!
     * \brief Determine if the CID can be found
     *
     * \param CID The CID to look for
     *
     * \return True if a block with the given CID was found, false otherwise
     */
    bool Has(const CCID& cid);

    /*!
     * \brief Find a block based on its CID
     *
     * \param cid The CID to look for
     * \param block The block, or unmodified if this returns false
     *
     * \return True if the block was retrieved, false for missing CID or error
     */
    bool Get(const CCID& cid, CBlock& block);

    /*!
     * \brief Add a block to the block store, replacing any existing block
     *
     * \param block The block to store
     *
     * \return True if the block was added, false on error
     */
    bool Put(const CBlock& block);

    /*!
     * \brief Delete a block based on its CID
     *
     * \param CID The CID to look for
     *
     * \return True on success, false on error
     */
    bool Delete(const CCID& cid);

  private:
    // Construction parameters
    CDataStore& m_dataStore;
  };
}
}
