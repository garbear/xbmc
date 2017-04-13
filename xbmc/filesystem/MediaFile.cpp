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

#include "MediaFile.h"
#include "File.h"
#include "media/MediaContainer.h"
#include "media/MediaStore.h"
#include "media/MediaStream.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"
#include "URL.h"

#include <algorithm>

using namespace XFILE;

#define MEDIA_STORE_PATH  "mediastore://"

CMediaFile::CMediaFile() :
  m_position(-1)
{
}

bool CMediaFile::Open(const CURL& url)
{
  unsigned int mediaId = GetMediaID(url.Get());
  m_position = 0;
  if (mediaId > 0)
  {
    m_mediaStream = CServiceBroker::GetMediaStore().Open(mediaId);
    return m_mediaStream.get() != nullptr;
  }

  return false;
}

bool CMediaFile::Exists(const CURL& url)
{
  return CServiceBroker::GetMediaStore().IsPinned(GetMediaID(url.Get()));
}

int CMediaFile::Stat(const CURL& url, struct __stat64* buffer)
{
  return CServiceBroker::GetMediaStore().Stat(GetMediaID(url.Get()), buffer) ? 0 : -1;
}

int CMediaFile::Stat(struct __stat64* buffer)
{
  return m_mediaStream->Stat(buffer) ? 0 : -1;
}

ssize_t CMediaFile::Read(void* bufPtr, size_t bufSize)
{
  uint8_t* buffer = static_cast<uint8_t*>(bufPtr);
  int64_t read = m_mediaStream->Read(m_position, buffer, bufSize);
  if (read > 0)
    m_position += read;

  return static_cast<ssize_t>(read);
}

bool CMediaFile::ReadString(char *szLine, int iLineLength)
{
  return false;
}

int64_t CMediaFile::Seek(int64_t iFilePosition, int iWhence /* = SEEK_SET */)
{
  int64_t newPos = -1;
  switch (iWhence)
  {
  case SEEK_CUR:
    newPos = m_position + iFilePosition;
    break;
  case SEEK_END:
    newPos = iFilePosition + std::max(GetLength(), (int64_t)0);
    break;
  case SEEK_SET:
    newPos = iFilePosition;
    break;
  default:
    break;
  }

  if (newPos >= 0)
  {
    m_position = newPos;
    return m_position;
  }

  return -1;
}

void CMediaFile::Close()
{
  m_mediaStream.reset();
  m_position = -1;
}

int64_t CMediaFile::GetPosition()
{
  return m_position;
}

int64_t CMediaFile::GetLength()
{
  return m_mediaStream->GetLength();
}

int CMediaFile::GetChunkSize()
{
  return m_mediaStream->GetChunkSize();
}

double CMediaFile::GetDownloadSpeed()
{
  return m_mediaStream->GetDownloadSpeed();
}

int CMediaFile::IoControl(EIoControl request, void* param)
{
  bool bSuccess = false;

  if (m_mediaStream)
  {
    switch (request)
    {
    case IOCTRL_SEEK_POSSIBLE:
      return 1;
    case IOCTRL_CACHE_STATUS:
    {
      SCacheStatus* status = static_cast<SCacheStatus*>(param);
      m_mediaStream->CacheStatus(status->forward,
                                 status->maxrate,
                                 status->currate,
                                 status->lowspeed);
      bSuccess = true;
      break;
    }
    case IOCTRL_CACHE_SETRATE:
    {
      if (m_mediaStream->SetWriteRate(*static_cast<unsigned*>(param)))
        bSuccess = true;
      break;
    }
    default:
      break;
    }
  }

  return bSuccess ? 0 : -1;
}

std::string CMediaFile::GetContent()
{
  return m_mediaStream->Container().GetMimeType();
}

unsigned int CMediaFile::GetMediaID(const std::string& mediaStorePath)
{
  return static_cast<unsigned int>(StringUtils::ToUint64(mediaStorePath.substr(strlen(MEDIA_STORE_PATH)), 0));
}

std::string CMediaFile::GetMediaStorePath(unsigned int mediaId)
{
  return MEDIA_STORE_PATH + StringUtils::Format("%u", mediaId);
}
