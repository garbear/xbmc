/*
 *      Copyright (C) 2017 Unique Digital Ltd.
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "MediaStoreDatabase.h"

using namespace KODI;
using namespace MEDIA;

//! @todo
//#define MEDIA_ID_START  (1 << 30)  // To avoid conflicts with internal db
#define MEDIA_ID_START  0

#define DATABASE_PATH  "special://temp/mediastore/index.xml"

CMediaStoreDatabase::CMediaStoreDatabase() :
  m_maxMediaId(MEDIA_ID_START)
{
}

CMediaStoreDatabase::~CMediaStoreDatabase()
{
}

void CMediaStoreDatabase::Initialize()
{
  Load();
}

void CMediaStoreDatabase::Deinitialize()
{
  Save();
}

int CMediaStoreDatabase::PinMedia(const std::string& path)
{
  Media needle(path);

  auto it = m_pinnedByPath.find(needle);
  if (it != m_pinnedByPath.end())
    return it->mediaId;

  int mediaId = ++m_maxMediaId;

  Media newMedia(mediaId, std::move(needle.path));

  m_pinnedById.insert(newMedia);
  m_pinnedByPath.insert(std::move(newMedia));

  Save();

  return mediaId;
}

bool CMediaStoreDatabase::IsPinned(unsigned int mediaId) const
{
  Media needle(mediaId);
  return m_pinnedById.find(needle) != m_pinnedById.end();
}

bool CMediaStoreDatabase::IsPinned(const std::string& path) const
{
  Media needle(path);
  return m_pinnedByPath.find(needle) != m_pinnedByPath.end();
}

std::string CMediaStoreDatabase::GetPath(unsigned int mediaId) const
{
  Media needle(mediaId);

  auto it = m_pinnedById.find(needle);
  if (it != m_pinnedById.end())
    return it->path;

  return "";
}

unsigned int CMediaStoreDatabase::GetMediaID(const std::string& path) const
{
  Media needle(path);

  auto it = m_pinnedByPath.find(needle);
  if (it != m_pinnedByPath.end())
    return it->mediaId;

  return 0;
}

void CMediaStoreDatabase::UnpinMedia(unsigned int mediaId)
{
  Media needle(mediaId);

  auto it = m_pinnedById.find(needle);
  if (it != m_pinnedById.end())
  {
    m_pinnedByPath.erase(*it);
    m_pinnedById.erase(it);
  }

  Save();
}

void CMediaStoreDatabase::UnpinMedia(const std::string& path)
{
  Media needle(path);

  auto it = m_pinnedByPath.find(needle);
  if (it != m_pinnedByPath.end())
  {
    m_pinnedById.erase(*it);
    m_pinnedByPath.erase(it);
  }

  Save();
}

void CMediaStoreDatabase::Load()
{
  //! @todo
}

void CMediaStoreDatabase::Save()
{
  //! @todo
}
