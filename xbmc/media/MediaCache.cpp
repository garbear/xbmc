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

#include "MediaCache.h"
#include "filesystem/CacheStrategy.h"
#include "filesystem/File.h"
#include "filesystem/IFileTypes.h"
#include "filesystem/SpecialProtocol.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <algorithm>
#include <sys/stat.h>

using namespace KODI;
using namespace MEDIA;

CMediaCache::CMediaCache(const std::string& localPath) :
  m_localPath(localPath),
  m_fileSize(-1),
  m_writePosition(-1),
  m_readPosition(-1)
{
}

CMediaCache::~CMediaCache()
{
  Close();
}

bool CMediaCache::Open()
{
  CSingleLock lockRead(m_writeMutex);
  CSingleLock lockWrite(m_readMutex);

  m_cache.reset(new XFILE::CSimpleFileCache(CSpecialProtocol::TranslatePath(m_localPath)));
  if (m_cache->Open() != CACHE_RC_OK)
  {
    CLog::LogF(LOGERROR, "Failed to open cache: %s", m_localPath.c_str());
    Close();
    return false;
  }

  m_cacheFile.reset(new XFILE::CFile);
  if (!m_cacheFile->Open(m_localPath, XFILE::READ_NO_CACHE))
  {
    CLog::LogF(LOGERROR, "Failed to open cache file: %s", m_localPath.c_str());
    Close();
    return false;
  }

  m_fileSize = m_cacheFile->GetLength();
  m_readPosition = 0;
  m_writePosition = -1;

  return true;
}

void CMediaCache::Close()
{
  CSingleLock lockRead(m_writeMutex);
  CSingleLock lockWrite(m_readMutex);

  m_cache.reset();
  m_cacheFile.reset();
}

int64_t CMediaCache::Read(uint64_t position, uint8_t* buffer, unsigned int bufferSize)
{
  CSingleLock lock(m_readMutex);

  if (m_cache)
  {
    if (position != m_readPosition)
    {
      int64_t newReadPos = m_cache->Seek(position);
      if (newReadPos < 0)
        return -1;

      m_readPosition = newReadPos;
    }

    if (position == m_readPosition)
    {
      int64_t totalSize = m_fileSize;
      if (m_writePosition > 0)
        totalSize = m_writePosition;

      if (static_cast<int64_t>(position) >= totalSize)
        return -1;

      int64_t available = totalSize - m_readPosition;
      int64_t toRead = std::min((int64_t)bufferSize, available);

      int64_t read = -1;
      if (toRead > 0)
        read = m_cache->ReadFromCache(buffer, static_cast<size_t>(toRead));

      if (read > 0)
        m_readPosition += read;

      return read;
    }
  }

  return -1;
}

bool CMediaCache::Stat(struct __stat64* buffer)
{
  if (m_cacheFile)
    return m_cacheFile->Stat(buffer) == 0;

  return false;
}

int CMediaCache::GetChunkSize() const
{
  if (m_cacheFile)
    return m_cacheFile->GetChunkSize();

  return 0;
}

void CMediaCache::CacheStatus(uint64_t& forward, unsigned& maxrate, unsigned& currate, bool& lowspeed) const
{
  //! @todo
  forward = 0;
  maxrate = 0;
  currate = 0;
  lowspeed = false;

  if (m_writePosition >= 0 && m_readPosition >= 0 && m_writePosition > m_readPosition)
    forward = std::max(m_writePosition - m_readPosition, (int64_t)0);
}

bool CMediaCache::WriteOutputPacket(const uint8_t *buffer, size_t size)
{
  CSingleLock lock(m_writeMutex);

  if (m_cache)
  {
    int written = m_cache->WriteToCache(buffer, size);
    if (written > 0)
    {
      if (m_writePosition < 0)
        m_writePosition = 0;

      int64_t temp = m_writePosition + written;
      m_writePosition = temp;
      m_fileSize = temp;

      return written == size;
    }
  }

  return false;
}

int64_t CMediaCache::SeekWrite(int64_t position, int whence)
{
  CSingleLock lock(m_writeMutex);

  if (m_cache)
  {
    if (whence == SEEK_SET && position < 0)
    {
      CLog::LogF(LOGDEBUG, "Failed to seek to offset %d (%d)", position, whence);
    }
    else
    {
      int64_t newPos = m_cache->SeekWrite(position, whence);

      if (newPos < 0)
        return -1;

      m_writePosition = newPos;
      return newPos;
    }
  }

  return -1;
}
