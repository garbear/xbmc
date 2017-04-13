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

#include <stdint.h>
#include <vector>

class CDemuxStream;
struct DemuxPacket;

namespace KODI
{
namespace MEDIA
{

class IMuxer
{
public:
  IMuxer() = default;
  virtual ~IMuxer() = default;

  virtual bool Open(const std::vector<CDemuxStream*>& streams) = 0;
  virtual void Close() = 0;
  virtual bool Write(const DemuxPacket& packet, CDemuxStream *stream) = 0;
  virtual int64_t Seek(int64_t position, int whence) = 0;
  virtual bool Aborted() = 0;

  //! @todo tag info
};

}
}
