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

#include "GenericDocumentStore.h"
#include "IKeyValueStore.h"
#include "utils/BSONVariantParser.h"
#include "utils/BSONVariantWriter.h"

#include <assert.h>

using namespace dbiplus;

CGenericDocumentStore::CGenericDocumentStore(IKeyValueStore* datastore) :
  m_datastore(datastore)
{
  assert(m_datastore);
}

CGenericDocumentStore::~CGenericDocumentStore(void)
{
  Disconnect();
  delete m_datastore;
}

bool CGenericDocumentStore::Connect(void)
{
  return m_datastore->Connect();
}

void CGenericDocumentStore::Disconnect(void)
{
  m_datastore->Disconnect();
}

bool CGenericDocumentStore::Put(const CVariant& key, const CVariant& value)
{
  std::string strKey = CBSONVariantWriter::Write(key);
  std::string strValue = CBSONVariantWriter::Write(value);

  return m_datastore->Put(strKey, strValue);
}

bool CGenericDocumentStore::Get(const CVariant& key, CVariant& value)
{
  std::string strKey = CBSONVariantWriter::Write(key);

  std::string strValue;
  if (m_datastore->Get(strKey, strValue))
  {
    value = CBSONVariantParser::Parse(strValue.c_str(), strValue.length());
    return true;
  }

  return false;
}

bool CGenericDocumentStore::Delete(const CVariant& key)
{
  return m_datastore->Delete(CBSONVariantWriter::Write(key));
}
