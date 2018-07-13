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

CBlockStore::CBlockStore(CDataStore& dataStore) :
  m_dataStore(dataStore)
{
}

bool CBlockStore::Has(const CCID& cid)
{
  std::vector<uint8_t> key = cid.Serialize();

  if (m_dataStore.Has(key.data(), key.size()))
    return true;

  return false;
}

bool CBlockStore::Get(const CCID& cid, CBlock& block)
{
  std::vector<uint8_t> key = cid.Serialize();

  if (m_dataStore.Get(key.data(), key.size(), block.Data()))
  {
    block.SetCID(cid);
    return true;
  }

  return false;
}

bool CBlockStore::Put(const CBlock& block)
{
  std::vector<uint8_t> key = block.CID().Serialize();

  if (m_dataStore.Put(key.data(), key.size(), block.Data().data(), block.Data().size()))
    return true;

  return false;
}

bool CBlockStore::Delete(const CCID& cid)
{
  std::vector<uint8_t> key = cid.Serialize();

  if (m_dataStore.Delete(key.data(), key.size()))
    return true;

  return false;
}
