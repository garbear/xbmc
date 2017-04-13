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
#pragma once

#include "threads/CriticalSection.h"
#include "PlatformDefs.h" // for __stat64

#include <atomic>
#include <memory>
#include <stdint.h>
#include <string>
#include <sys/stat.h>

namespace XFILE
{
  class CCacheStrategy;
  class CFile;
}

namespace KODI
{
namespace MEDIA
{
class CMediaCache
{
public:
  CMediaCache(const std::string& localPath);
  virtual ~CMediaCache();

  const std::string& GetLocalPath() const { return m_localPath; }

  bool Open();
  void Close();

  // Read interface
  int64_t Read(uint64_t position, uint8_t* buffer, unsigned int bufferSize);
  bool Stat(struct __stat64* buffer);
  int64_t GetLength() const { return m_fileSize; }
  int GetChunkSize() const;
  double GetDownloadSpeed() const { return 0.0; } //! @todo
  void CacheStatus(uint64_t& forward, unsigned& maxrate, unsigned& currate, bool& lowspeed) const;
  bool SetWriteRate(unsigned int writeRate) { return false; } //! @todo

  // Write interface
  bool WriteOutputPacket(const uint8_t *buffer, size_t size);
  int64_t SeekWrite(int64_t position, int whence);

private:
  // Construction parameters
  const std::string m_localPath; //! \brief Path to local cache

  // Local cache properties
  std::unique_ptr<XFILE::CCacheStrategy> m_cache;
  std::unique_ptr<XFILE::CFile> m_cacheFile;
  std::atomic<int64_t> m_fileSize;
  std::atomic<int64_t> m_writePosition;
  int64_t m_readPosition;
  CCriticalSection m_readMutex;
  CCriticalSection m_writeMutex;

};

}
}
