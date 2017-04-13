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

#include "MediaStore.h"
#include "FileStore.h"
#include "MediaContainer.h"
#include "MediaStoreDatabase.h"
#include "MediaStream.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "filesystem/MediaFile.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"

#include <algorithm>
#include <cstring>
#include <sys/stat.h>

using namespace KODI;
using namespace MEDIA;

#define LOCAL_CACHE_PATH  "special://home/mediastore/"
#define MKV_EXTENSION     ".mkv"

CMediaStore::CMediaStore() :
  m_database(new CMediaStoreDatabase),
  m_fileStore(new CFileStore),
  m_bInitialized(false)
{
}

CMediaStore::~CMediaStore()
{
  Deinitialize();
}

void CMediaStore::Initialize()
{
  if (!XFILE::CDirectory::Exists(LOCAL_CACHE_PATH))
    XFILE::CDirectory::Create(LOCAL_CACHE_PATH);

  m_database->Initialize();
  m_fileStore->Initialize();

  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
  m_bInitialized = true;
}

void CMediaStore::Deinitialize()
{
  if (m_bInitialized)
  {
    CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
    m_bInitialized = false;
  }

  m_fileStore->Deinitialize();
  m_database->Deinitialize();
}

int CMediaStore::PinMedia(const std::string& path)
{
  int mediaId = m_database->PinMedia(path);
  m_fileStore->PinMedia(mediaId, path);
  return mediaId;
}

bool CMediaStore::IsPinned(unsigned int mediaId) const
{
  return m_database->IsPinned(mediaId);
}

bool CMediaStore::IsPinned(const std::string& path) const
{
  return m_database->IsPinned(path);
}

unsigned int CMediaStore::GetMediaID(const std::string& path) const
{
  return m_database->GetMediaID(path);
}

void CMediaStore::UnpinMedia(unsigned int mediaId)
{
  m_fileStore->UnpinMedia(mediaId);
  m_database->UnpinMedia(mediaId);
}

std::shared_ptr<CMediaStream> CMediaStore::Open(unsigned int mediaId)
{
  std::shared_ptr<CMediaStream> stream;

  std::string mediaPath = m_database->GetPath(mediaId);
  if (!mediaPath.empty())
    stream = m_fileStore->Open(mediaId, mediaPath);

  return stream;
}

bool CMediaStore::Stat(unsigned int mediaId, struct __stat64* buffer)
{
  std::string mediaPath = m_database->GetPath(mediaId);
  if (!mediaPath.empty())
    return XFILE::CFile::Stat(CMediaStore::BuildLocalPath(mediaPath), buffer) == 0;

  return false;
}

void CMediaStore::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (strcmp(sender, "xbmc") == 0)
  {
    if (strcmp(message, "OnQuit") == 0)
      Deinitialize();
  }
}

std::string CMediaStore::BuildLocalPath(const std::string& mediaPath)
{
  return LOCAL_CACHE_PATH + CUtil::GetFileDigest(mediaPath, KODI::UTILITY::CDigest::Type::MD5) + MKV_EXTENSION;
}
