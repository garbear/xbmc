/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "threads/CriticalSection.h"

#include <memory>
#include <string>

class CVariant;

namespace dbiplus
{
  class IDocumentStore;
}

class CNoSqlDatabase
{
public:
  CNoSqlDatabase(void) { }
  ~CNoSqlDatabase(void) { }

  virtual const char* GetBaseDBName() const = 0;

  bool Connect(void);
  void Disconnect(void);

  bool Put(const CVariant& key, const CVariant& value);
  bool Get(const CVariant& key, CVariant& value);
  bool Delete(const CVariant& key);
  //bool CreateSnapshot(void);
  //bool WriteBatch(void);
  //bool RangeIteration(void);

  // CustomComparators
  // Sync/Async

protected:
  std::string GetDatabasePath(void) const;
  
  std::unique_ptr<dbiplus::IDocumentStore> m_pDS;

private:
  unsigned int     m_openCount;
  CCriticalSection m_openMutex;
};
