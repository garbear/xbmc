/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileCache.h"

#include "CircularCache.h"
#include "File.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/Thread.h"
#include "utils/log.h"

#include <mutex>

#if !defined(TARGET_WINDOWS)
#include "platform/posix/ConvUtils.h"
#endif

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <inttypes.h>
#include <memory>
#include <stdexcept>

#ifdef TARGET_POSIX
#include "platform/posix/ConvUtils.h"
#endif

using namespace XFILE;

namespace
{
class CFileCacheSource final : public IFileCacheSource
{
public:
  bool Open(const CURL& url, unsigned int flags) override { return m_file.Open(url.Get(), flags); }
  void Abort() override { m_file.Abort(); }
  void Close() override { m_file.Close(); }
  ssize_t Read(void* buffer, size_t size) override { return m_file.Read(buffer, size); }
  int64_t Seek(int64_t position, int whence) override { return m_file.Seek(position, whence); }
  int64_t GetLength() override { return m_file.GetLength(); }
  int GetChunkSize() override { return m_file.GetChunkSize(); }
  int IoControl(IOControl request, void* param) override
  {
    return m_file.IoControl(request, param);
  }
  IFile* GetImplementation() override { return m_file.GetImplementation(); }

private:
  CFile m_file;
};
} // namespace

class CWriteRate
{
public:
  CWriteRate() : m_stamp(std::chrono::steady_clock::now()), m_time(std::chrono::milliseconds(0))
  {
    m_pos   = 0;
    m_size = 0;
  }

  void Reset(int64_t pos, bool bResetAll = true)
  {
    m_stamp = std::chrono::steady_clock::now();
    m_pos   = pos;

    if (bResetAll)
    {
      m_size  = 0;
      m_time = std::chrono::milliseconds(0);
    }
  }

  uint32_t Rate(int64_t pos, uint32_t time_bias = 0)
  {
    auto ts = std::chrono::steady_clock::now();

    m_size += (pos - m_pos);
    m_time += std::chrono::duration_cast<std::chrono::milliseconds>(ts - m_stamp);
    m_pos = pos;
    m_stamp = ts;

    if (m_time == std::chrono::milliseconds(0))
      return 0;

    return static_cast<uint32_t>(1000 * (m_size / (m_time.count() + time_bias)));
  }

private:
  std::chrono::time_point<std::chrono::steady_clock> m_stamp;
  int64_t  m_pos;
  std::chrono::milliseconds m_time;
  int64_t  m_size;
};

CFileCache::CFileCache(const unsigned int flags)
  : CFileCache(flags, std::make_unique<CFileCacheSource>())
{
}

CFileCache::CFileCache(unsigned int flags, std::unique_ptr<IFileCacheSource> source)
  : CThread("FileCache"),
    m_source(std::move(source)),
    m_fileSize(0),
    m_flags(flags)
{
  if (!m_source)
    throw std::invalid_argument("FileCache source must not be null");
}

CFileCache::~CFileCache()
{
  Close();
}

IFile *CFileCache::GetFileImp()
{
  return m_source->GetImplementation();
}

bool CFileCache::Open(const CURL& url)
{
  std::unique_lock lock(m_sync);

  const uint64_t openGeneration = BeginOpen();
  if (IsOpenCancelled(openGeneration))
  {
    SetLastError(ECANCELED);
    return false;
  }

  {
    std::unique_lock seekLock(m_seekSync);
    m_abortRequested = false;
    m_seekGeneration = 0;
    m_seekCompletedGeneration = 0;
    m_nSeekResult = 0;
    m_seekError = 0;
  }

  m_sourcePath = url.GetRedacted();

  CLog::Log(LOGDEBUG, "CFileCache::{} - <{}> opening", __FUNCTION__, m_sourcePath);

  // Opening the source file.
  // The READ_NO_CACHE and READ_NO_BUFFER flags are required to avoid create other instances of
  // FileCache or StreamBuffer since CFile::Open is called again in loop
  m_sourceActive = true;
  if (IsOpenCancelled(openGeneration))
  {
    Close();
    SetLastError(ECANCELED);
    return false;
  }

  if (!m_source->Open(url, READ_NO_CACHE | READ_TRUNCATED | READ_NO_BUFFER))
  {
    const bool cancelled = IsOpenCancelled(openGeneration);
    CLog::Log(LOGERROR, "CFileCache::{} - <{}> failed to open", __FUNCTION__, m_sourcePath);
    Close();
    if (cancelled)
      SetLastError(ECANCELED);
    return false;
  }

  if (IsOpenCancelled(openGeneration))
  {
    Close();
    SetLastError(ECANCELED);
    return false;
  }

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (!settings)
  {
    const bool cancelled = IsOpenCancelled(openGeneration);
    Close();
    if (cancelled)
      SetLastError(ECANCELED);
    return false;
  }

  const unsigned int cacheMemSize =
      settings->GetInt(CSettings::SETTING_FILECACHE_MEMORYSIZE) * 1024 * 1024;

  m_source->IoControl(IOControl::SET_CACHE, this);

  bool retry = false;
  m_source->IoControl(IOControl::SET_RETRY, &retry); // We already handle retrying ourselves

  // check if source can seek
  m_seekPossible = m_source->IoControl(IOControl::SEEK_POSSIBLE, NULL);

  // Determine the best chunk size we can use
  m_chunkSize = CFile::DetermineChunkSize(m_source->GetChunkSize(),
                                          settings->GetInt(CSettings::SETTING_FILECACHE_CHUNKSIZE));
  CLog::Log(LOGDEBUG,
            "CFileCache::{} - <{}> source chunk size is {}, setting cache chunk size to {}",
            __FUNCTION__, m_sourcePath, m_source->GetChunkSize(), m_chunkSize);

  m_fileSize = m_source->GetLength();

  if (IsOpenCancelled(openGeneration))
  {
    Close();
    SetLastError(ECANCELED);
    return false;
  }

  std::unique_ptr<CCacheStrategy> cache;
  if (!m_pCache)
  {
    if (cacheMemSize == 0)
    {
      // Use cache on disk
      cache = std::make_unique<CSimpleFileCache>();
      m_forwardCacheSize = 0;
      m_maxForward = m_fileSize;
    }
    else
    {
      size_t cacheSize;
      if (m_fileSize > 0 && m_fileSize < cacheMemSize && !(m_flags & READ_AUDIO_VIDEO))
      {
        // Cap cache size by filesize, but not for audio/video files as those may grow.
        // We don't need to take into account READ_MULTI_STREAM here as that's only used for audio/video
        cacheSize = m_fileSize;

        // Cap chunk size by cache size
        if (m_chunkSize > cacheSize)
          m_chunkSize = cacheSize;
      }
      else
      {
        cacheSize = cacheMemSize;

        // NOTE: READ_MULTI_STREAM is only used with READ_AUDIO_VIDEO
        if (m_flags & READ_MULTI_STREAM)
        {
          // READ_MULTI_STREAM requires double buffering, so use half the amount of memory for each buffer
          cacheSize /= 2;
        }

        // Make sure cache can at least hold 2 chunks
        if (cacheSize < m_chunkSize * 2)
          cacheSize = m_chunkSize * 2;
      }

      if (m_flags & READ_MULTI_STREAM)
        CLog::Log(LOGDEBUG, "CFileCache::{} - <{}> using double memory cache each sized {} bytes",
                  __FUNCTION__, m_sourcePath, cacheSize);
      else
        CLog::Log(LOGDEBUG, "CFileCache::{} - <{}> using single memory cache sized {} bytes",
                  __FUNCTION__, m_sourcePath, cacheSize);

      const size_t back = cacheSize / 4;
      const size_t front = cacheSize - back;

      cache = std::make_unique<CCircularCache>(front, back);
      m_forwardCacheSize = front;
      m_maxForward = m_forwardCacheSize;
    }

    if (m_flags & READ_MULTI_STREAM)
    {
      // If READ_MULTI_STREAM flag is set: Double buffering is required
      cache = std::make_unique<CDoubleCache>(cache.release());
    }
  }

  // open cache strategy
  bool cacheOpened = false;
  {
    std::unique_lock seekLock(m_seekSync);
    if (!IsOpenCancelled(openGeneration))
    {
      if (cache)
        m_pCache = std::move(cache);
      cacheOpened = m_pCache && m_pCache->Open() == CACHE_RC_OK;
      if (cacheOpened && !IsOpenCancelled(openGeneration))
        m_pCache->ClearEndOfInput();
      else
        cacheOpened = false;
    }
  }

  if (!cacheOpened)
  {
    const bool cancelled = IsOpenCancelled(openGeneration);
    if (!cancelled)
      CLog::Log(LOGERROR, "CFileCache::{} - <{}> failed to open cache", __FUNCTION__, m_sourcePath);
    Close();
    if (cancelled)
      SetLastError(ECANCELED);
    return false;
  }

  m_readPos = 0;
  m_writePos = 0;
  m_writeRate = 1024 * 1024;
  m_writeRateActual = 0;
  m_writeRateLowSpeed = 0;
  m_bFilling = true;
  m_sourcePositionValid = true;
  m_seekEvent.Reset();
  m_seekEnded.Reset();

  bool createCancelled;
  {
    std::unique_lock stopLock(m_stopSync);
    createCancelled = IsOpenCancelled(openGeneration);
    if (!createCancelled)
      CThread::Create(false);
  }

  if (createCancelled)
  {
    Close();
    SetLastError(ECANCELED);
    return false;
  }

  if (IsOpenCancelled(openGeneration))
  {
    Close();
    SetLastError(ECANCELED);
    return false;
  }

  return true;
}

void CFileCache::Process()
{
  if (!m_pCache)
  {
    CLog::Log(LOGERROR, "CFileCache::{} - <{}> sanity failed. no cache strategy", __FUNCTION__,
              m_sourcePath);
    return;
  }

  // create our read buffer
  std::unique_ptr<char[]> buffer(new char[m_chunkSize]);
  if (buffer == nullptr)
  {
    CLog::Log(LOGERROR, "CFileCache::{} - <{}> failed to allocate read buffer", __FUNCTION__,
              m_sourcePath);
    return;
  }

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (!settings)
    return;

  float readFactor = settings->GetInt(CSettings::SETTING_FILECACHE_READFACTOR) / 100.0f;

  const bool useAdaptativeReadFactor = (readFactor < 1.0f);

  CWriteRate limiter;
  CWriteRate average;

  while (!m_bStop)
  {
    bool seekRequested = false;
    if (m_sourcePositionValid)
    {
      m_fileSize = m_source->GetLength();
      seekRequested = m_seekEvent.Wait(0ms);
    }
    else
    {
      if (AbortableWait(m_seekEvent) != WAIT_SIGNALED || m_bStop)
        break;
      seekRequested = true;
    }

    if (m_bStop)
      break;

    // check for seek events
    if (seekRequested)
    {
      uint64_t seekGeneration;
      int64_t seekPos;
      {
        std::unique_lock seekLock(m_seekSync);
        if (m_seekCompletedGeneration >= m_seekGeneration)
          continue;

        seekGeneration = m_seekGeneration;
        seekPos = m_seekPos;
      }

      const int64_t cacheMaxPos = m_pCache->CachedDataEndPosIfSeekTo(seekPos);
      const bool cacheReachEOF = (cacheMaxPos == m_fileSize);

      bool sourceSeekFailed = false;
      int64_t seekResult = seekPos;
      DWORD seekError = 0;
      if (!cacheReachEOF || !m_sourcePositionValid)
      {
        const int64_t sourceSeekResult = m_source->Seek(cacheMaxPos, SEEK_SET);
        const DWORD sourceSeekError = GetLastError();

        if (m_abortRequested || m_bStop)
          break;

        if (sourceSeekResult != cacheMaxPos)
        {
          CLog::Log(LOGERROR, "CFileCache::{} - <{}> error {} seeking. Seek returned {}",
                    __FUNCTION__, m_sourcePath,
                    sourceSeekError != 0 ? sourceSeekError : static_cast<DWORD>(EIO),
                    sourceSeekResult);
          seekResult = -1;
          seekError = sourceSeekError != 0 ? sourceSeekError : EIO;
          m_seekPossible = m_source->IoControl(IOControl::SEEK_POSSIBLE, NULL);
          sourceSeekFailed = true;
          m_sourcePositionValid = false;

          {
            std::unique_lock seekLock(m_seekSync);
            if (m_pCache->Seek(m_readPos) != m_readPos)
            {
              const bool completeReset = m_pCache->Reset(m_readPos);
              m_writePos = m_pCache->CachedDataEndPos();
              average.Reset(m_writePos, completeReset);
              limiter.Reset(m_writePos);
              if (completeReset)
              {
                m_bFilling = true;
                m_writeRateLowSpeed = 0;
              }
            }
            m_pCache->EndOfInput();
          }
        }
      }

      if (!sourceSeekFailed)
      {
        bool bCompleteReset;
        {
          std::unique_lock seekLock(m_seekSync);
          if (m_abortRequested || seekGeneration != m_seekGeneration ||
              m_seekCompletedGeneration >= seekGeneration)
            break;
          m_pCache->ClearEndOfInput();
          bCompleteReset = m_pCache->Reset(seekPos);
          m_readPos = seekPos;
          m_writePos = m_pCache->CachedDataEndPos();
        }

        assert(m_writePos == cacheMaxPos);
        average.Reset(m_writePos, bCompleteReset); // Can only recalculate new average from scratch after a full reset (empty cache)
        limiter.Reset(m_writePos);
        m_sourcePositionValid = true;
        if (bCompleteReset)
        {
          CLog::Log(LOGDEBUG,
                    "CFileCache::{} - <{}> cache completely reset for seek to position {}",
                    __FUNCTION__, m_sourcePath, seekPos);
          m_bFilling = true;
          m_writeRateLowSpeed = 0;
        }
      }

      bool publishResult = false;
      {
        std::unique_lock seekLock(m_seekSync);
        if (!m_abortRequested && seekGeneration == m_seekGeneration &&
            m_seekCompletedGeneration < seekGeneration)
        {
          m_nSeekResult = seekResult;
          m_seekError = seekError;
          m_seekCompletedGeneration = seekGeneration;
          publishResult = true;
        }
      }
      if (publishResult)
        m_seekEnded.Set();

      if (sourceSeekFailed)
        continue;
    }

    // variable read factor based on cache level
    if (useAdaptativeReadFactor)
    {
      // cache level [0.0 - 1.0]
      const double level = static_cast<double>(m_writePos - m_readPos) / m_maxForward;
      readFactor = static_cast<float>(level * -2.5 + 4.0); // read factor [4.0x - 1.5x]
    }

    while (m_writeRate)
    {
      if (m_writePos - m_readPos < m_writeRate * readFactor)
      {
        limiter.Reset(m_writePos);
        break;
      }

      if (limiter.Rate(m_writePos) < m_writeRate * readFactor)
        break;

      if (m_seekEvent.Wait(m_processWait))
      {
        if (!m_bStop)
          m_seekEvent.Set();
        break;
      }
    }

    if (m_bStop)
      break;

    const int64_t maxWrite = m_pCache->GetMaxWriteSize(m_chunkSize);
    int64_t maxSourceRead = m_chunkSize;
    // Cap source read size by space available between current write position and EOF
    if (m_fileSize != 0)
      maxSourceRead = std::min(maxSourceRead, m_fileSize - m_writePos);

    /* Only read from source if there's enough write space in the cache
     * else we may keep disposing data and seeking back on (slow) source
     */
    if (maxWrite < maxSourceRead)
    {
      // Wait until sufficient cache write space is available
      m_pCache->m_space.Wait(5ms);
      continue;
    }

    ssize_t iRead = 0;
    if (maxSourceRead > 0)
      iRead = m_source->Read(buffer.get(), maxSourceRead);

    if (m_bStop)
      break;

    if (iRead <= 0)
    {
      // Check for actual EOF and retry as long as we still have data in our cache
      if (m_writePos < m_fileSize && m_pCache->WaitForData(0, 0ms) > 0)
      {
        CLog::Log(LOGWARNING, "CFileCache::{} - <{}> source read returned {}! Will retry",
                  __FUNCTION__, m_sourcePath, iRead);

        // Wait a bit:
        if (m_seekEvent.Wait(2000ms))
        {
          if (!m_bStop)
            m_seekEvent.Set(); // hack so that later we realize seek is needed
        }

        // and retry:
        continue; // while (!m_bStop)
      }
      else
      {
        if (iRead < 0)
          CLog::Log(LOGERROR,
                    "{} - <{}> source read failed with {}!", __FUNCTION__, m_sourcePath, iRead);
        else if (m_fileSize == 0)
          CLog::Log(LOGDEBUG,
                    "CFileCache::{} - <{}> source read didn't return any data! Hit eof(?)",
                    __FUNCTION__, m_sourcePath);
        else if (m_writePos < m_fileSize)
          CLog::Log(LOGERROR,
                    "CFileCache::{} - <{}> source read didn't return any data before eof!",
                    __FUNCTION__, m_sourcePath);
        else
          CLog::Log(LOGDEBUG, "CFileCache::{} - <{}> source read hit eof", __FUNCTION__,
                    m_sourcePath);

        m_pCache->EndOfInput();

        // The thread event will now also cause the wait of an event to return a false.
        if (AbortableWait(m_seekEvent) == WAIT_SIGNALED)
        {
          std::unique_lock seekLock(m_seekSync);
          if (m_bStop)
            break;
          m_pCache->ClearEndOfInput();
          m_seekEvent.Set(); // hack so that later we realize seek is needed
        }
        else
          break; // while (!m_bStop)
      }
    }

    int iTotalWrite = 0;
    while (!m_bStop && (iTotalWrite < iRead))
    {
      int iWrite = 0;
      iWrite = m_pCache->WriteToCache(buffer.get() + iTotalWrite, iRead - iTotalWrite);

      // write should always work. all handling of buffering and errors should be
      // done inside the cache strategy. only if unrecoverable error happened, WriteToCache would return error and we break.
      if (iWrite < 0)
      {
        CLog::Log(LOGERROR, "CFileCache::{} - <{}> error writing to cache", __FUNCTION__,
                  m_sourcePath);
        m_bStop = true;
        break;
      }
      else if (iWrite == 0)
      {
        m_pCache->m_space.Wait(5ms);
      }

      iTotalWrite += iWrite;

      // check if seek was asked. otherwise if cache is full we'll freeze.
      if (m_seekEvent.Wait(0ms))
      {
        if (!m_bStop)
          m_seekEvent.Set(); // make sure we get the seek event later.
        break;
      }
    }

    m_writePos += iTotalWrite;

    // under estimate write rate by a second, to
    // avoid uncertainty at start of caching
    m_writeRateActual = average.Rate(m_writePos, 1000);

   /* NOTE: We can only reliably test for low speed condition, when the cache is *really*
    * filling. This is because as soon as it's full the average-
    * rate will become approximately the current-rate which can flag false
    * low read-rate conditions.
    */
    if (m_bFilling && m_forwardCacheSize != 0)
    {
      const int64_t forward = m_pCache->WaitForData(0, 0ms);
      if (forward + m_chunkSize >= m_forwardCacheSize)
      {
        if (m_writeRateActual < m_writeRate)
          m_writeRateLowSpeed = m_writeRateActual;

        m_bFilling = false;
      }
    }
  }
}

void CFileCache::OnExit()
{
  m_bStop = true;
  CancelPendingOperations(false);
}

bool CFileCache::Exists(const CURL& url)
{
  return CFile::Exists(url.Get());
}

int CFileCache::Stat(const CURL& url, struct __stat64* buffer)
{
  return CFile::Stat(url.Get(), buffer);
}

ssize_t CFileCache::Read(void* lpBuf, size_t uiBufSize)
{
  std::unique_lock lock(m_sync);
  if (!m_pCache)
  {
    CLog::Log(LOGERROR, "CFileCache::{} - <{}> sanity failed. no cache strategy!", __FUNCTION__,
              m_sourcePath);
    return -1;
  }

  if (m_abortRequested)
  {
    SetLastError(ECANCELED);
    return -1;
  }
  int64_t iRc;

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

retry:
  // attempt to read
  iRc = m_pCache->ReadFromCache((char *)lpBuf, uiBufSize);
  if (iRc > 0)
  {
    m_readPos += iRc;
    return (int)iRc;
  }

  if (iRc == CACHE_RC_WOULD_BLOCK)
  {
    if (!m_sourcePositionValid)
    {
      SetLastError(m_seekError != 0 ? m_seekError : EIO);
      return -1;
    }

    // just wait for some data to show up
    iRc = m_pCache->WaitForData(1, 10s);
    if (m_abortRequested)
    {
      SetLastError(ECANCELED);
      return -1;
    }
    if (iRc > 0)
      goto retry;
    if (!m_sourcePositionValid)
    {
      SetLastError(m_seekError != 0 ? m_seekError : EIO);
      return -1;
    }
  }

  if (iRc == CACHE_RC_TIMEOUT)
  {
    CLog::Log(LOGWARNING, "CFileCache::{} - <{}> timeout waiting for data", __FUNCTION__,
              m_sourcePath);
    return -1;
  }

  if (iRc == 0)
  {
    if (!m_sourcePositionValid)
    {
      SetLastError(m_seekError != 0 ? m_seekError : EIO);
      return -1;
    }
    return 0;
  }

  // unknown error code
  CLog::Log(LOGERROR, "CFileCache::{} - <{}> cache strategy returned unknown error code {}",
            __FUNCTION__, m_sourcePath, (int)iRc);
  return -1;
}

int64_t CFileCache::Seek(int64_t iFilePosition, int iWhence)
{
  std::unique_lock lock(m_sync);

  if (!m_pCache)
  {
    CLog::Log(LOGERROR, "CFileCache::{} - <{}> sanity failed. no cache strategy!", __FUNCTION__,
              m_sourcePath);
    return -1;
  }

  if (m_abortRequested)
  {
    SetLastError(ECANCELED);
    return -1;
  }

  int64_t iCurPos = m_readPos;
  int64_t iTarget = iFilePosition;
  if (iWhence == SEEK_END)
    iTarget = m_fileSize + iTarget;
  else if (iWhence == SEEK_CUR)
    iTarget = iCurPos + iTarget;
  else if (iWhence != SEEK_SET)
    return -1;

  if (iTarget == m_readPos && m_sourcePositionValid)
    return m_readPos;

  const bool sourcePositionInvalid = !m_sourcePositionValid;
  const int64_t cacheSeekResult = m_pCache->Seek(iTarget);
  if (m_abortRequested)
  {
    SetLastError(ECANCELED);
    return -1;
  }

  if (cacheSeekResult != iTarget || sourcePositionInvalid)
  {
    if (m_seekPossible == 0)
    {
      if (sourcePositionInvalid && cacheSeekResult == iTarget)
        m_pCache->Seek(m_readPos);
      if (sourcePositionInvalid)
      {
        SetLastError(m_seekError != 0 ? m_seekError : EIO);
        return -1;
      }
      return cacheSeekResult;
    }

    // Never request closer to end than one chunk. Speeds up tag reading
    const int64_t seekPos = std::min(iTarget, std::max((int64_t)0, m_fileSize - m_chunkSize));
    uint64_t seekGeneration;
    {
      std::unique_lock seekLock(m_seekSync);
      if (m_abortRequested || !CThread::IsRunning())
      {
        SetLastError(ECANCELED);
        return -1;
      }

      seekGeneration = ++m_seekGeneration;
      m_seekPos = seekPos;
      m_nSeekResult = -1;
      m_seekError = 0;
      m_seekEnded.Reset();
    }
    m_seekEvent.Set();

    int64_t seekResult;
    DWORD seekError;
    while (true)
    {
      const bool seekSignaled = m_seekEnded.Wait(100ms);

      std::unique_lock seekLock(m_seekSync);
      if (m_seekCompletedGeneration >= seekGeneration)
      {
        seekResult = m_nSeekResult;
        seekError = m_seekError;
        break;
      }
      if (!seekSignaled && !CThread::IsRunning())
      {
        seekResult = -1;
        seekError = m_abortRequested ? ECANCELED : EIO;
        break;
      }
    }

    if (seekResult != seekPos)
    {
      SetLastError(seekError);
      return -1;
    }

    /* wait for any remaining data */
    if (seekPos < iTarget)
    {
      CLog::Log(LOGDEBUG, "CFileCache::{} - <{}> waiting for position {}", __FUNCTION__,
                m_sourcePath, iTarget);
      if (m_pCache->WaitForData(static_cast<uint32_t>(iTarget - seekPos), 10s) < iTarget - seekPos)
      {
        if (m_abortRequested)
        {
          SetLastError(ECANCELED);
          return -1;
        }
        CLog::Log(LOGWARNING, "CFileCache::{} - <{}> failed to get remaining data", __FUNCTION__,
                  m_sourcePath);
        return -1;
      }

      if (m_abortRequested)
      {
        SetLastError(ECANCELED);
        return -1;
      }
      m_pCache->Seek(iTarget);
    }
    m_readPos = iTarget;
  }
  else
    m_readPos = iTarget;

  return iTarget;
}

void CFileCache::Close()
{
  BeginClose();
  StopThread();

  std::unique_lock lock(m_sync);
  if (m_pCache)
  {
    std::unique_lock seekLock(m_seekSync);
    m_pCache->Close();
  }

  m_sourceActive = false;
  m_source->Close();

  {
    std::unique_lock seekLock(m_seekSync);
    m_abortRequested = false;
  }
  EndClose();
}

void CFileCache::Abort()
{
  RequestAbort();
  StopThread(false);
}

uint64_t CFileCache::BeginOpen()
{
  std::unique_lock lock(m_lifecycleSync);
  return m_lifecycleGeneration;
}

bool CFileCache::IsOpenCancelled(uint64_t generation)
{
  std::unique_lock lock(m_lifecycleSync);
  return m_explicitAbortRequested || m_closeRequests != 0 || m_lifecycleGeneration != generation;
}

void CFileCache::BeginClose()
{
  std::unique_lock lock(m_lifecycleSync);
  ++m_closeRequests;
  ++m_lifecycleGeneration;
}

void CFileCache::EndClose()
{
  std::unique_lock lock(m_lifecycleSync);
  if (--m_closeRequests == 0)
    m_explicitAbortRequested = false;
}

void CFileCache::RequestAbort()
{
  std::unique_lock lock(m_lifecycleSync);
  m_explicitAbortRequested = true;
  ++m_lifecycleGeneration;
}

int64_t CFileCache::GetPosition()
{
  return m_readPos;
}

int64_t CFileCache::GetLength()
{
  return m_fileSize;
}

void CFileCache::StopThread(bool bWait /*= true*/)
{
  std::unique_lock lock(m_stopSync);
  CThread::StopThread(false);
  CancelPendingOperations(true);
  if (bWait)
    CThread::StopThread(true);
}

void CFileCache::CancelPendingOperations(bool abortSource)
{
  const bool firstAbort = !m_abortRequested.exchange(true);

  {
    std::unique_lock seekLock(m_seekSync);
    if (m_seekCompletedGeneration < m_seekGeneration)
    {
      m_nSeekResult = -1;
      m_seekError = ECANCELED;
      m_seekCompletedGeneration = m_seekGeneration;
    }

    if (m_pCache)
      m_pCache->EndOfInput();
  }

  m_seekEvent.Set();
  m_seekEnded.Set();

  if (abortSource && firstAbort && m_sourceActive)
    m_source->Abort();
}

const std::string CFileCache::GetProperty(XFILE::FileProperty type, const std::string &name) const
{
  if (!m_source->GetImplementation())
    return IFile::GetProperty(type, name);

  return m_source->GetImplementation()->GetProperty(type, name);
}

int CFileCache::IoControl(IOControl request, void* param)
{
  if (request == IOControl::CACHE_STATUS)
  {
    SCacheStatus* status = (SCacheStatus*)param;
    status->maxforward = m_maxForward;
    status->forward = m_pCache->WaitForData(0, 0ms);
    status->maxrate = m_writeRate;
    status->currate = m_writeRateActual;
    status->lowrate = m_writeRateLowSpeed;
    m_writeRateLowSpeed = 0; // Reset low speed condition
    return 0;
  }

  if (request == IOControl::CACHE_SETRATE)
  {
    m_writeRate = *static_cast<uint32_t*>(param);

    const double mBits = m_writeRate / 1024.0 / 1024.0 * 8.0; // Mbit/s

    // calculates wait time inversely proportional to the bitrate
    // and limited between 30 - 100 ms
    const int wait = std::clamp(static_cast<int>(110.0 - mBits), 30, 100);

    m_processWait = std::chrono::milliseconds(wait);

    CLog::Log(LOGDEBUG,
              "CFileCache::IoControl - setting maxRate to {:.2f} Mbit/s with processWait of {} ms",
              mBits, wait);
    return 0;
  }

  if (request == IOControl::SEEK_POSSIBLE)
    return m_seekPossible;

  return -1;
}
