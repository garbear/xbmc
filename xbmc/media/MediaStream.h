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

#include "cores/VideoPlayer/IVideoPlayer.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "FileItem.h"

#include <memory>
#include <sys/stat.h>

class CDVDInputStream;
class CDVDDemux;
struct DemuxPacket;

namespace XFILE
{
  class CSimpleFileCache;
}

namespace KODI
{
namespace MEDIA
{
class CMediaCache;
class CMediaContainer;
class IMuxer;

class CMediaStream : public IVideoPlayer,
                     protected CThread
{
public:
  CMediaStream(const std::string& mediaPath);
  virtual ~CMediaStream();

  const std::string& GetLocalPath() const { return m_cachePath; }
  
  bool Open();
  void Close();

  // Container information
  CMediaContainer& Container() { return *m_container; }
  const CMediaContainer& Container() const { return *m_container; }

  // Read interface
  int64_t Read(uint64_t position, uint8_t* buffer, unsigned int bufferSize);
  bool Stat(struct __stat64* buffer);
  int64_t GetLength() const;
  int GetChunkSize() const;
  double GetDownloadSpeed() const;
  void CacheStatus(uint64_t& forward, unsigned& maxrate, unsigned& currate, bool& lowspeed) const;
  bool SetWriteRate(unsigned int writeRate);

  // implementation of IVideoPlayer
  virtual int OnDiscNavResult(void* pData, int iMessage) override { return -1; }
  virtual void GetVideoResolution(unsigned int &width, unsigned int &height) override;

protected:
  // implementation of CThread
  virtual void Process() override;

private:
  // Handle packet being transfered from demuxer to muxer
  bool OnPacket(const DemuxPacket& packet);

  // Construction parameters
  const std::string m_mediaPath; //! \brief Path to media
  const std::string m_cachePath; //! \brief Path to local cache

  // Container information
  std::unique_ptr<CMediaContainer> m_container;

  // Local cache properties
  std::unique_ptr<CMediaCache> m_cache;

  // Stream properties
  std::shared_ptr<CDVDInputStream> m_inputStream;
  std::unique_ptr<CDVDDemux> m_demuxer;

  // Muxer
  std::unique_ptr<IMuxer> m_muxer;
};

}
}
