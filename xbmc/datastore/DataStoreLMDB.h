/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDataStore.h"

#include <map>
#include <utility>

struct MDB_env;
struct MDB_txn;

namespace KODI
{
namespace DATASTORE
{
  class CDataStoreLMDB : public IDataStore
  {
  public:
    ~CDataStoreLMDB() override = default;

    // Implementation of IDataStore
    const char* Name() const override;
    bool Open(const std::string& dataStorePath) override;
    void Close() override;
    bool Has(const uint8_t* key, size_t keySize) override;
    bool Get(const uint8_t* key, size_t keySize, std::vector<uint8_t>& data) override;
    uint8_t* Reserve(const uint8_t* key, size_t keySize, size_t dataSize) override;
    void Commit(const uint8_t* data) override;
    bool Put(const uint8_t* key, size_t keySize, const uint8_t* data, size_t dataSize) override;
    bool Delete(const uint8_t* key, size_t keySize) override;

  private:
    MDB_txn* CreateTransaction();
    void CommitTransaction(MDB_txn* transaction);
    bool OpenDataStore(MDB_txn* transaction, unsigned int &databaseHandle);
    void CloseDataStore(unsigned int databaseHandle);

    // LMDB parameters
    MDB_env* m_environment = nullptr;
    using DataStoreHandle = std::pair<MDB_txn*, unsigned int>;
    std::map<const void*, DataStoreHandle> m_reservedData;
  };
}
}
