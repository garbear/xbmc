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

#include "LevelDbDatastore.h"

#include <leveldb/db.h>

using namespace dbiplus;

CLevelDbDatastore::CLevelDbDatastore(const std::string& strPath) :
  m_strPath(strPath)
{
}

bool CLevelDbDatastore::Connect(void)
{
  Disconnect();
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, m_strPath.c_str(), &m_db);
  return status.ok();
}

void CLevelDbDatastore::Disconnect(void)
{
  delete m_db;
  m_db = NULL;
}

bool CLevelDbDatastore::Put(const std::string& key, const std::string& value)
{
  if (!m_db)
    return false;

  leveldb::Status status = m_db->Put(leveldb::WriteOptions(), leveldb::Slice(key), value);
  return status.ok();
}

bool CLevelDbDatastore::Get(const std::string& key, std::string& value)
{
  if (!m_db)
    return false;
  
  leveldb::Status status = m_db->Get(leveldb::ReadOptions(), leveldb::Slice(key), &value);
  return status.ok();
}

bool CLevelDbDatastore::Delete(const std::string& key)
{
  if (!m_db)
    return false;
  
  leveldb::Status status = m_db->Delete(leveldb::WriteOptions(), leveldb::Slice(key));
  return status.ok();
}
