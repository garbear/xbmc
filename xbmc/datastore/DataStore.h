/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

class CProfileManager;

namespace KODI
{
namespace DATASTORE
{
  class IDataStore;

  class CDataStore
  {
  public:
    CDataStore(const CProfileManager& profileManager);
    ~CDataStore();

    // Wrappers for IDataStore functions
    bool Open();
    void Close();
    bool Has(const uint8_t* key, size_t keySize);
    bool Get(const uint8_t* key, size_t keySize, std::vector<uint8_t>& data);
    uint8_t* Reserve(const uint8_t* key, size_t keySize, size_t dataSize);
    void Commit(const uint8_t* data);
    bool Put(const uint8_t* key, size_t keySize, const uint8_t* data, size_t dataSize);
    bool Delete(const uint8_t* key, size_t keySize);

  private:
    // Factory function
    std::unique_ptr<IDataStore> CreateDataStore();

    // Construction parameters
    const CProfileManager &m_profileManager;

    // Data store parameters
    std::unique_ptr<IDataStore> m_dataStore;
  };
}
}
