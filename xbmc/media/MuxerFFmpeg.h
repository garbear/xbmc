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

#include "IMuxer.h"

#include <memory>
#include <vector>

struct AVFormatContext;
class CDemuxStream;
struct DemuxPacket;

namespace KODI
{
namespace MEDIA
{
class CMediaCache;

class CMuxerFFmpeg : public IMuxer
{
public:
  CMuxerFFmpeg(CMediaCache *callback, int inputBlockSize = 0);
  virtual ~CMuxerFFmpeg();

  // implementation of IMuxer
  virtual bool Open(const std::vector<CDemuxStream*>& streams) override;
  virtual void Close() override;
  virtual bool Write(const DemuxPacket& packet, CDemuxStream *stream) override;
  virtual int64_t Seek(int64_t position, int whence) override;
  virtual bool Aborted() override { return m_bAborted; }

  // Receive output packet
  void WriteOutputPacket(const uint8_t *buffer, int size);

private:
  struct delete_format_context
  {
    void operator()(AVFormatContext *formatContext) const;
  };

  typedef std::unique_ptr<AVFormatContext, delete_format_context> AVFormatContextPtr;

  bool AddStream(const CDemuxStream *stream);

  // Construction parameters
  CMediaCache *const m_callback;
  const int m_inputBlockSize;

  // libav properties
  AVFormatContextPtr m_formatContext;

  bool m_bAborted;
};

}
}
