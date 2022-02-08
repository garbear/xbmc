/*
 *  Copyright (C) 2018-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DataStoreLMDB.h"

#include "filesystem/Directory.h"
#include "utils/log.h"

#include <lmdb.h>

using namespace KODI;
using namespace DATASTORE;

namespace
{
bool IsValidPointerSizePair(const uint8_t* data, size_t size)
{
  return data != nullptr || size == 0;
}
} // namespace

/*!
 * \brief The size of the memory map to use for the LMDB environment
 *
 * \sa mdb_env_set_mapsize()
 */
#define CACHE_SIZE (1UL * 1024UL * 1024UL * 1024UL)

bool CDataStoreLMDB::Open(const std::string& dataStorePath)
{
  // Create path if it doesn't exist
  if (!XFILE::CDirectory::Exists(dataStorePath, true))
  {
    if (!XFILE::CDirectory::Create(dataStorePath))
    {
      CLog::Log(LOGERROR, "Failed to create LMDB directory: {}", dataStorePath);
      return false;
    }
  }

  if (mdb_env_create(&m_environment) != 0)
  {
    CLog::Log(LOGERROR, "Failed to create LMDB environment");
    return false;
  }

  // Set the cache size
  if (mdb_env_set_mapsize(m_environment, CACHE_SIZE) != 0)
  {
    CLog::Log(LOGERROR, "Failed to set the LMDB cache size to {} bytes", CACHE_SIZE);
    Close();
    return false;
  }

  int result = mdb_env_open(m_environment, dataStorePath.c_str(), 0, 0664);
  if (result != 0)
  {
    switch (result)
    {
      case MDB_VERSION_MISMATCH:
        CLog::Log(LOGERROR, "The version of the LMDB library doesn't match the version that "
                            "created the database environment");
        break;
      case MDB_INVALID:
        CLog::Log(LOGERROR, "The LMDB environment file headers are corrupted");
        break;
      case ENOENT:
        CLog::Log(LOGERROR, "The LMDB directory doesn't exist: {}", dataStorePath);
        break;
      case EACCES:
        CLog::Log(LOGERROR,
                  "The user doesn't have permission to access the LMDB environment files");
        break;
      case EAGAIN:
        CLog::Log(LOGERROR, "The LMDB environment is locked by another process");
        break;
      default:
        break;
    }

    Close();
    return false;
  }

  return true;
}

void CDataStoreLMDB::Close()
{
  if (m_environment != nullptr)
  {
    for (const auto& getData : m_getData)
    {
      AbortTransaction(getData.second.first);
      CloseDataStore(getData.second.second);
    }
    m_getData.clear();

    for (const auto& reservedData : m_reservedData)
    {
      AbortTransaction(reservedData.second.first);
      CloseDataStore(reservedData.second.second);
    }
    m_reservedData.clear();

    mdb_env_close(m_environment);
    m_environment = nullptr;
  }
}

bool CDataStoreLMDB::Has(const uint8_t* key, size_t keySize)
{
  if (!IsValidPointerSizePair(key, keySize))
  {
    CLog::Log(LOGERROR, "LMDB: Cannot check key with size {} from a null pointer", keySize);
    return false;
  }

  const uint8_t* dummyData = nullptr;
  size_t dummySize = 0;
  if (Get(key, keySize, dummyData, dummySize))
  {
    Release(dummyData);
    return true;
  }

  return false;
}

bool CDataStoreLMDB::Get(const uint8_t* key, size_t keySize, const uint8_t*& data, size_t& dataSize)
{
  if (!IsValidPointerSizePair(key, keySize))
  {
    CLog::Log(LOGERROR, "LMDB: Cannot get key with size {} from a null pointer", keySize);
    return false;
  }

  MDB_txn* transaction = CreateTransaction(true);
  if (transaction == nullptr)
    return false;

  MDB_dbi databaseHandle;
  if (!OpenDataStore(transaction, static_cast<unsigned int&>(databaseHandle)))
  {
    AbortTransaction(transaction);
    return false;
  }

  // Get the data
  MDB_val databaseKey{keySize, const_cast<uint8_t*>(key)};
  MDB_val databaseData{};
  const int result = mdb_get(transaction, databaseHandle, &databaseKey, &databaseData);
  if (result != 0)
  {
    switch (result)
    {
      case MDB_NOTFOUND:
      {
        //! The key was not in the database
        break;
      }
      case EINVAL:
      {
        CLog::Log(LOGERROR, "LMDB: An invalid parameter was specified");
        break;
      }
      default:
        break;
    }

    AbortTransaction(transaction);
    CloseDataStore(databaseHandle);
    return false;
  }

  // Success
  data = static_cast<const uint8_t*>(databaseData.mv_data);
  dataSize = databaseData.mv_size;
  m_getData[data] = std::make_pair(transaction, static_cast<unsigned int>(databaseHandle));

  return true;
}

void CDataStoreLMDB::Release(const uint8_t* data)
{
  auto it = m_getData.find(data);
  if (it != m_getData.end())
  {
    MDB_txn* transaction;
    unsigned int databaseHandle;
    std::tie(transaction, databaseHandle) = it->second;

    AbortTransaction(transaction);
    CloseDataStore(static_cast<MDB_dbi>(databaseHandle));

    m_getData.erase(it);
  }
}

uint8_t* CDataStoreLMDB::Reserve(const uint8_t* key, size_t keySize, size_t dataSize)
{
  if (!IsValidPointerSizePair(key, keySize))
  {
    CLog::Log(LOGERROR, "LMDB: Cannot reserve key with size {} from a null pointer", keySize);
    return nullptr;
  }

  MDB_txn* transaction = CreateTransaction(false);
  if (transaction == nullptr)
    return nullptr;

  MDB_dbi databaseHandle;
  if (!OpenDataStore(transaction, static_cast<unsigned int&>(databaseHandle)))
  {
    AbortTransaction(transaction);
    return nullptr;
  }

  // Reserve the data
  MDB_val databaseKey{keySize, const_cast<uint8_t*>(key)};
  MDB_val databaseData{dataSize, nullptr};
  int result = mdb_put(transaction, databaseHandle, &databaseKey, &databaseData, MDB_RESERVE);
  if (result != 0)
  {
    switch (result)
    {
      case MDB_MAP_FULL:
      {
        CLog::Log(LOGERROR, "LMDB: The database is full");
        //! @todo Adjust size. See mdb_env_set_mapsize()
        break;
      }
      case MDB_TXN_FULL:
      {
        CLog::Log(LOGERROR, "LMDB: The transaction has too many dirty pages");
        break;
      }
      case EACCES:
      {
        CLog::Log(LOGERROR, "LMDB: An attempt was made to write in a read-only transaction");
        break;
      }
      case EINVAL:
      {
        CLog::Log(LOGERROR,
                  "LMDB: An invalid parameter was specified while writing to the database");
        break;
      }
      default:
        break;
    }

    AbortTransaction(transaction);
    CloseDataStore(databaseHandle);
    return nullptr;
  }

  // Success
  uint8_t* data = static_cast<uint8_t*>(databaseData.mv_data);
  m_reservedData[data] = std::make_pair(transaction, static_cast<unsigned int>(databaseHandle));

  return data;
}

void CDataStoreLMDB::Commit(const uint8_t* data)
{
  auto it = m_reservedData.find(data);
  if (it != m_reservedData.end())
  {
    const DataStoreHandle& handle = it->second;

    CommitTransaction(handle.first);
    CloseDataStore(static_cast<MDB_dbi>(handle.second));

    m_reservedData.erase(it);
  }
}

bool CDataStoreLMDB::Put(const uint8_t* key, size_t keySize, const uint8_t* data, size_t dataSize)
{
  if (!IsValidPointerSizePair(key, keySize))
  {
    CLog::Log(LOGERROR, "LMDB: Cannot put key with size {} from a null pointer", keySize);
    return false;
  }

  if (!IsValidPointerSizePair(data, dataSize))
  {
    CLog::Log(LOGERROR, "LMDB: Cannot put data with size {} from a null pointer", dataSize);
    return false;
  }

  MDB_txn* transaction = CreateTransaction(false);
  if (transaction == nullptr)
    return false;

  MDB_dbi databaseHandle;
  if (!OpenDataStore(transaction, static_cast<unsigned int&>(databaseHandle)))
  {
    AbortTransaction(transaction);
    return false;
  }

  // Add the data
  MDB_val databaseKey{keySize, const_cast<uint8_t*>(key)};
  MDB_val databaseData{dataSize, const_cast<uint8_t*>(data)};
  const int result = mdb_put(transaction, databaseHandle, &databaseKey, &databaseData, 0);
  if (result != 0)
  {
    switch (result)
    {
      case MDB_MAP_FULL:
      {
        CLog::Log(LOGERROR, "LMDB: The database is full");
        //! @todo Adjust size. See mdb_env_set_mapsize()
        break;
      }
      case MDB_TXN_FULL:
      {
        CLog::Log(LOGERROR, "LMDB: The transaction has too many dirty pages");
        break;
      }
      case EACCES:
      {
        CLog::Log(LOGERROR, "LMDB: An attempt was made to write in a read-only transaction");
        break;
      }
      case EINVAL:
      {
        CLog::Log(LOGERROR,
                  "LMDB: An invalid parameter was specified while writing to the database");
        break;
      }
      default:
        break;
    }

    AbortTransaction(transaction);
    CloseDataStore(databaseHandle);
    return false;
  }

  // Success
  CommitTransaction(transaction);
  CloseDataStore(databaseHandle);

  return true;
}

bool CDataStoreLMDB::Delete(const uint8_t* key, size_t keySize)
{
  if (!IsValidPointerSizePair(key, keySize))
  {
    CLog::Log(LOGERROR, "LMDB: Cannot delete key with size {} from a null pointer", keySize);
    return false;
  }

  MDB_txn* transaction = CreateTransaction(false);
  if (transaction == nullptr)
    return false;

  MDB_dbi databaseHandle;
  if (!OpenDataStore(transaction, static_cast<unsigned int&>(databaseHandle)))
  {
    AbortTransaction(transaction);
    return false;
  }

  MDB_val databaseKey{keySize, const_cast<uint8_t*>(key)};
  const int result = mdb_del(transaction, databaseHandle, &databaseKey, nullptr);
  if (result != 0)
  {
    switch (result)
    {
      case MDB_NOTFOUND:
      {
        //! The key was not in the database
        break;
      }
      case EINVAL:
      {
        CLog::Log(LOGERROR, "LMDB: An invalid parameter was specified while deleting");
        break;
      }
      default:
        break;
    }

    AbortTransaction(transaction);
    CloseDataStore(databaseHandle);
    return false;
  }

  CommitTransaction(transaction);
  CloseDataStore(databaseHandle);

  return true;
}

MDB_txn* CDataStoreLMDB::CreateTransaction(bool readOnly)
{
  MDB_txn* transaction = nullptr;

  if (m_environment != nullptr)
  {
    const unsigned int flags = readOnly ? MDB_RDONLY : 0;
    const int result = mdb_txn_begin(m_environment, NULL, flags, &transaction);
    if (result != 0)
    {
      switch (result)
      {
        case MDB_PANIC:
        {
          CLog::Log(LOGERROR,
                    "A fatal error occurred earlier in the LMDB environment. Closing it now.");
          // Shutdown LMDB environment
          Close();
          break;
        }
        case MDB_MAP_RESIZED:
        {
          CLog::Log(LOGERROR, "Another process wrote data beyond the LMDB mapsize");
          //! @todo This environment's map must be resized. See mdb_env_set_mapsize()
          break;
        }
        case MDB_READERS_FULL:
        {
          CLog::Log(LOGERROR,
                    "A read-only transaction was requested and the reader lock table is full");
          //! @todo Adjust reader lock table. See mdb_env_set_maxreaders()
          break;
        }
        case ENOMEM:
        {
          CLog::Log(LOGERROR, "LMDB: Out of memory");
          break;
        }
        default:
          break;
      }
    }
  }

  return transaction;
}

void CDataStoreLMDB::CommitTransaction(MDB_txn* transaction)
{
  const int result = mdb_txn_commit(transaction);
  if (result != 0)
  {
    switch (result)
    {
      case EINVAL:
      {
        CLog::Log(LOGERROR, "LMDB: Commit transaction: An invalid parameter was specified");
        break;
      }
      case ENOSPC:
      {
        CLog::Log(LOGERROR, "LMDB: Commit transaction: No more disk space");
        break;
      }
      case EIO:
      {
        CLog::Log(LOGERROR,
                  "LMDB: Commit transaction: A low-level I/O error occurred while writing");
        break;
      }
      case ENOMEM:
      {
        CLog::Log(LOGERROR, "LMDB: Commit transaction: Out of memory");
        break;
      }
      default:
        break;
    }
  }
}

void CDataStoreLMDB::AbortTransaction(MDB_txn* transaction)
{
  mdb_txn_abort(transaction);
}

bool CDataStoreLMDB::OpenDataStore(MDB_txn* transaction, unsigned int& databaseHandle)
{
  if (transaction == nullptr)
    return false;

  const int result = mdb_dbi_open(transaction, NULL, 0, static_cast<MDB_dbi*>(&databaseHandle));
  if (result != 0)
  {
    switch (result)
    {
      case MDB_NOTFOUND:
      {
        CLog::Log(LOGERROR, "The specified LMDB database doesn't exist in the environment");
        break;
      }
      case MDB_DBS_FULL:
      {
        CLog::Log(LOGERROR, "Too many LMDB databases have been opened");
        //! @todo Set max number of DBs for environment. See mdb_env_set_maxdbs()
        break;
      }
      default:
        break;
    }

    return false;
  }

  return true;
}

void CDataStoreLMDB::CloseDataStore(unsigned int databaseHandle)
{
  if (m_environment != nullptr)
    mdb_dbi_close(m_environment, static_cast<MDB_dbi>(databaseHandle));
}
