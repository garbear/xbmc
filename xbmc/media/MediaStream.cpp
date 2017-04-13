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

#include "MediaStream.h"
#include "MediaCache.h"
#include "MediaContainer.h"
#include "MediaStore.h"
#include "MuxerFFmpeg.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStream.h"
#include "cores/VideoPlayer/Interface/Addon/DemuxPacket.h"
#include "filesystem/CacheStrategy.h"
#include "filesystem/SpecialProtocol.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "PlatformDefs.h" // for __stat64
#include "URL.h"

#if defined(TARGET_POSIX)
  #include "platform/posix/filesystem/PosixFile.h"
  using CacheLocalFile = XFILE::CPosixFile;
#elif defined(TARGET_WINDOWS)
  #include "platform/win32/filesystem/Win32File.h"
  using CacheLocalFile = XFILE::CWin32File;
#endif

#include <algorithm>
#include <sys/stat.h>

using namespace KODI;
using namespace MEDIA;

#define SLEEP_DURATION_MS  100

CMediaStream::CMediaStream(const std::string& mediaPath) :
  CThread("MediaStream"),
  m_mediaPath(mediaPath),
  m_cachePath(CMediaStore::BuildLocalPath(mediaPath)),
  m_container(new CMediaContainer),
  m_cache(new CMediaCache(m_cachePath))
{
}

CMediaStream::~CMediaStream()
{
  Close();
}

bool CMediaStream::Open()
{
  // Open local cache
  if (!m_cache->Open())
  {
    Close();
    return false;
  }

  // Open input stream
  m_inputStream = CDVDFactoryInputStream::CreateInputStream(this, CFileItem(m_mediaPath, false));
  if (!m_inputStream)
  {
    CLog::Log(LOGERROR, "CMediaStream - unable to create input stream for [%s]", CURL::GetRedacted(m_mediaPath).c_str());
    Close();
    return false;
  }

  if (!m_inputStream->Open())
  {
    CLog::Log(LOGERROR, "CVideoPlayer::OpenInputStream - error opening [%s]", CURL::GetRedacted(m_mediaPath).c_str());
    Close();
    return false;
  }

  // Open demuxer
  m_demuxer.reset(CDVDFactoryDemuxer::CreateDemuxer(m_inputStream));
  if (!m_demuxer)
  {
    CLog::Log(LOGERROR, "CMediaStream - unable to create demuxer for [%s]", CURL::GetRedacted(m_mediaPath).c_str());
    Close();
    return false;
  }

  // Opem muxer
  m_muxer.reset(new CMuxerFFmpeg(m_cache.get(), m_inputStream->GetBlockSize()));
  if (!m_muxer->Open(m_demuxer->GetStreams()))
  {
    CLog::Log(LOGERROR, "CMediaStream - unable to create muxer for [%s]", CURL::GetRedacted(m_mediaPath).c_str());
    Close();
    return false;
  }

  Create();

  return true;
}

void CMediaStream::Close()
{
  StopThread(false);

  if (m_demuxer)
    m_demuxer->Abort();

  if (m_inputStream)
    m_inputStream->Abort();

  StopThread(true);

  m_muxer.reset();

  m_demuxer.reset();
  m_inputStream.reset();

  m_cache.reset();
}

void CMediaStream::Process()
{
  while (!m_bStop)
  {
    for (DemuxPacket* pPacket = m_demuxer->Read(); pPacket != nullptr; pPacket = m_demuxer->Read())
    {
      if (!OnPacket(*pPacket))
      {
        CLog::LogF(LOGERROR, "Failed to write packet of %d bytes", pPacket->iSize);
        m_bStop = true;
        break;
      }
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    }

    if (!m_bStop)
      Sleep(SLEEP_DURATION_MS);
  }
}

bool CMediaStream::OnPacket(const DemuxPacket& packet)
{
  CDemuxStream *stream = nullptr;

  if (m_demuxer)
    stream = m_demuxer->GetStream(packet.demuxerId, packet.iStreamId);

  return m_muxer->Write(packet, stream);
}

int64_t CMediaStream::Read(uint64_t position, uint8_t* buffer, unsigned int bufferSize)
{
  return m_cache->Read(position, buffer, bufferSize);
}

bool CMediaStream::Stat(struct __stat64* buffer)
{
  return m_cache->Stat(buffer);
}

int64_t CMediaStream::GetLength() const
{
  return m_cache->GetLength();
}

int CMediaStream::GetChunkSize() const
{
  return m_cache->GetChunkSize();
}

double CMediaStream::GetDownloadSpeed() const
{
  return m_cache->GetDownloadSpeed();
}

void CMediaStream::CacheStatus(uint64_t& forward, unsigned& maxrate, unsigned& currate, bool& lowspeed) const
{
  m_cache->CacheStatus(forward, maxrate, currate, lowspeed);
}

bool CMediaStream::SetWriteRate(unsigned int writeRate)
{
  return m_cache->SetWriteRate(writeRate);
}

void CMediaStream::GetVideoResolution(unsigned int &width, unsigned int &height)
{
  //! @todo
  width = 0;
  height = 0;
}
