/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "datastore/CID.h"

#include <cstddef>
#include <cstdint>

namespace KODI::DATASTORE
{
class CBlock;
class CBlockStore;
} // namespace KODI::DATASTORE

namespace XFILE::IPFS
{

class CIPFSBlockUtils
{
public:
  static bool MakeAddressedBlock(KODI::DATASTORE::CIDCodec codec,
                                 const uint8_t* data,
                                 size_t size,
                                 KODI::DATASTORE::CCID& cid,
                                 KODI::DATASTORE::CBlock& block);

  static bool StoreAddressedBlock(KODI::DATASTORE::CBlockStore& blockStore,
                                  KODI::DATASTORE::CIDCodec codec,
                                  const uint8_t* data,
                                  size_t size,
                                  KODI::DATASTORE::CCID& cid);
};

} // namespace XFILE::IPFS
