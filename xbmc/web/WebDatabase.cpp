/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebDatabase.h"
#include "ServiceBroker.h"
#include "dbwrappers/dataset.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace WEB;

CWebDatabase::CWebDatabase(void) = default;

CWebDatabase::~CWebDatabase(void) = default;

bool CWebDatabase::Open()
{
  return CDatabase::Open(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseWeb);
}

void CWebDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "CWebDatabase::%s: create favourites table", __FUNCTION__);
  m_pDS->exec("CREATE TABLE favourites ("
              "id integer primary key,"
              "name text,"
              "url text,"
              "icon text,"
              "dateAdded text)");
}

void CWebDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "CWebDatabase::%s: creating indices", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxFavourites ON favourites(name)");
}

void CWebDatabase::UpdateTables(int version)
{

}

bool CWebDatabase::AddFavourite(FavouriteEntryPtr entry)
{
  try
  {
    if (nullptr == m_pDB.get())
      return false;
    if (nullptr == m_pDS.get())
      return false;

    std::string sql = PrepareSQL("SELECT id FROM favourites WHERE name='%s' AND url='%s'",
                                  entry->GetName().c_str(),
                                  entry->GetURL().c_str());
    m_pDS->query(sql);
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      sql = PrepareSQL("INSERT INTO favourites(name, url, icon, dateAdded) "
                       "values('%s', '%s', '%s', '%s')",
                         entry->GetName().c_str(),
                         entry->GetURL().c_str(),
                         entry->GetIcon().c_str(),
                         entry->GetDateTimeAdded().GetAsDBDateTime().c_str());
      return ExecuteQuery(sql);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CWebDatabase::%s failed", __FUNCTION__);
  }
  return false;
}

bool CWebDatabase::RemoveFavourite(const std::string& name, const std::string& url)
{
  std::string sql = PrepareSQL("DELETE FROM favourites where name='%s' AND url='%s'",
                                  name.c_str(),
                                  url.c_str());
  return ExecuteQuery(sql);
}

bool CWebDatabase::GetFavourites(std::vector<FavouriteEntryPtr>& entries)
{
  try
  {
    if (nullptr == m_pDB.get())
      return false;
    if (nullptr == m_pDS.get())
      return false;

    m_pDS->query(PrepareSQL("SELECT * FROM favourites"));
    while (!m_pDS->eof())
    {
      FavouriteEntryPtr entry = std::make_shared<CFavouriteEntry>(m_pDS->fv(1).get_asString(),
                                                                  m_pDS->fv(2).get_asString(),
                                                                  m_pDS->fv(3).get_asString(),
                                                                  CDateTime::FromDBDateTime(m_pDS->fv(4).get_asString()));
      entries.emplace_back(entry);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CWebDatabase::%s failed", __FUNCTION__);
  }
  return true;
}

bool CWebDatabase::ClearFavourites()
{
  std::string sql = PrepareSQL("delete from favourites");
  return ExecuteQuery(sql);
}
