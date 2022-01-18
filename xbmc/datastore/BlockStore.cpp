/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlockStore.h"

#include "Block.h"
#include "CID.h"
#include "DataStore.h"

using namespace KODI;
using namespace DATASTORE;

CBlockStore::CBlockStore(CDataStore& dataStore) : m_dataStore(dataStore)
{
}

bool CBlockStore::Has(const CCID& cid)
{
  const uint8_t* key;
  size_t keySize;
  std::tie(key, keySize) = cid.Serialize();

  if (m_dataStore.Has(key, keySize))
    return true;

  return false;
}

bool CBlockStore::Get(const CCID& cid, CBlock& block)
{
  const uint8_t* key;
  size_t keySize;
  std::tie(key, keySize) = cid.Serialize();

  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  if (m_dataStore.Get(key, keySize, data, dataSize))
  {
    block.SetCID(cid);
    //block.SetData(data, dataSize);
    return true;
  }

  return false;
}

bool CBlockStore::Put(const CBlock& block)
{
  const uint8_t* key;
  size_t keySize;
  std::tie(key, keySize) = block.CID().Serialize();

  if (m_dataStore.Put(key, keySize, block.Data().data(), block.Data().size()))
    return true;

  return false;
}

bool CBlockStore::Delete(const CCID& cid)
{
  const uint8_t* key;
  size_t keySize;
  std::tie(key, keySize) = cid.Serialize();

  if (m_dataStore.Delete(key, keySize))
    return true;

  return false;
}
