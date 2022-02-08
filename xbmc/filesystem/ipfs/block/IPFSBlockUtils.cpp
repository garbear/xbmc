/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPFSBlockUtils.h"

#include "crypto/multiformats/Multihash.h"
#include "datastore/Block.h"
#include "datastore/BlockStore.h"

#include <utility>
#include <vector>

using namespace KODI;
using namespace XFILE::IPFS;

bool CIPFSBlockUtils::MakeAddressedBlock(DATASTORE::CIDCodec codec,
                                         const uint8_t* data,
                                         size_t size,
                                         DATASTORE::CCID& cid,
                                         DATASTORE::CBlock& block)
{
  if (data == nullptr && size != 0)
    return false;

  CRYPTO::CMultihash cryptoMultihash{CRYPTO::CMultihash::Identifier::SHA2_256, {}};
  cryptoMultihash.Update(data, size);
  cryptoMultihash.Finalize();

  std::vector<uint8_t> serializedMultihash;
  if (!cryptoMultihash.Serialize(serializedMultihash))
    return false;

  DATASTORE::CCID newCid{codec, std::move(serializedMultihash)};
  std::vector<uint8_t> blockData;
  if (data != nullptr && size != 0)
    blockData.assign(data, data + size);

  DATASTORE::CBlock newBlock{newCid, std::move(blockData)};

  cid = std::move(newCid);
  block = std::move(newBlock);
  return true;
}

bool CIPFSBlockUtils::StoreAddressedBlock(DATASTORE::CBlockStore& blockStore,
                                          DATASTORE::CIDCodec codec,
                                          const uint8_t* data,
                                          size_t size,
                                          DATASTORE::CCID& cid)
{
  DATASTORE::CCID newCid;
  DATASTORE::CBlock block;
  if (!MakeAddressedBlock(codec, data, size, newCid, block) || !blockStore.Put(block))
    return false;

  cid = std::move(newCid);
  return true;
}
