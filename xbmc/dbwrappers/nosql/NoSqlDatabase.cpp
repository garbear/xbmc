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

#include "NoSqlDatabase.h"
#include "GenericDocumentStore.h"
#include "LevelDbDatastore.h"
#include "DatabaseManager.h"
#include "filesystem/SpecialProtocol.h"
#include "profiles/ProfilesManager.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

using namespace dbiplus;

bool CNoSqlDatabase::Connect(void)
{
  // check our database manager to see if this database can be opened
  if (!CDatabaseManager::Get().CanOpen(GetBaseDBName()))
    return false;

  CSingleLock lock(m_openMutex);
  if (m_openCount == 0)
  {
    m_pDS.reset(new CGenericDocumentStore(new CLevelDbDatastore(GetDatabasePath())));
    
    if (!m_pDS->Connect())
      return false;
  }

  m_openCount++;

  return true;
}

void CNoSqlDatabase::Disconnect(void)
{
  CSingleLock lock(m_openMutex);
  if (m_openCount > 0 && --m_openCount == 0)
  {
    m_pDS->Disconnect();
    m_pDS.reset();
  }
}

bool CNoSqlDatabase::Put(const CVariant& key, const CVariant& value)
{
  if (!m_pDS)
    return false;

  return m_pDS->Put(key, value);
}

bool CNoSqlDatabase::Get(const CVariant& key, CVariant& value)
{
  if (!m_pDS)
    return false;

  return m_pDS->Get(key, value);
}

bool CNoSqlDatabase::Delete(const CVariant& key)
{
  if (!m_pDS)
    return false;

  return m_pDS->Delete(key);
}

std::string CNoSqlDatabase::GetDatabasePath(void) const
{
  std::string strDir = CSpecialProtocol::TranslatePath(CProfilesManager::Get().GetDatabaseFolder());
  return URIUtils::AddFileToFolder(strDir, GetBaseDBName()) + ".db";
}
