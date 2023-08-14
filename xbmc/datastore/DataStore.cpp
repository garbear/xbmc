/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DataStore.h"

#include "DataStoreLMDB.h"
#include "IDataStore.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace DATASTORE;

CDataStore::~CDataStore()
{
  Close();
}

bool CDataStore::Open(const std::string& dataStorePath)
{
  // Validate parameters
  if (dataStorePath.empty())
    return false;

  std::unique_ptr<IDataStore> dataStore = CreateDataStore();
  if (!dataStore)
    return false;

  CLog::Log(LOGINFO, "Opening data store at {}", CURL::GetRedacted(dataStorePath));
  if (!dataStore->Open(dataStorePath))
    return false;

  m_dataStore = std::move(dataStore);
  return true;
}

void CDataStore::Close()
{
  if (m_dataStore)
  {
    m_dataStore->Close();
    m_dataStore.reset();
  }
}

bool CDataStore::Has(const uint8_t* key, size_t keySize)
{
  if (m_dataStore)
    return m_dataStore->Has(key, keySize);

  return false;
}

bool CDataStore::Get(const uint8_t* key, size_t keySize, std::vector<uint8_t>& data)
{
  if (m_dataStore)
    return m_dataStore->Get(key, keySize, data);

  return false;
}

uint8_t* CDataStore::Reserve(const uint8_t* key, size_t keySize, size_t dataSize)
{
  if (m_dataStore)
    return m_dataStore->Reserve(key, keySize, dataSize);

  return nullptr;
}

void CDataStore::Commit(const uint8_t* data)
{
  if (m_dataStore)
    m_dataStore->Commit(data);
}

bool CDataStore::Put(const uint8_t* key, size_t keySize, const uint8_t* data, size_t dataSize)
{
  if (m_dataStore)
    return m_dataStore->Put(key, keySize, data, dataSize);

  return false;
}

bool CDataStore::Delete(const uint8_t* key, size_t keySize)
{
  if (m_dataStore)
    return m_dataStore->Delete(key, keySize);

  return false;
}

std::unique_ptr<IDataStore> CDataStore::CreateDataStore()
{
  return std::make_unique<CDataStoreLMDB>();
}
