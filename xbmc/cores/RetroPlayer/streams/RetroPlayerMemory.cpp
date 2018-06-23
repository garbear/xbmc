/*
 *      Copyright (C) 2012-2017 Team Kodi
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

#include "RetroPlayerMemory.h"

#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerMemory::CRetroPlayerMemory()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[MEMORY]: Initializing memory stream");
}

CRetroPlayerMemory::~CRetroPlayerMemory()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[MEMORY]: Deinitializing memory stream");

  CloseStream();
}

bool CRetroPlayerMemory::OpenStream(const StreamProperties& properties)
{
  const MemoryStreamProperties& memoryProperties =
      reinterpret_cast<const MemoryStreamProperties&>(properties);

  if (m_bOpen)
  {
    CloseStream();
    m_bOpen = false;
  }

  const size_t size = memoryProperties.size;

  CLog::Log(LOGDEBUG, "RetroPlayer[MEMORY]: Creating memory stream - size %u", size);

  //! @todo
  /*
  if (m_renderManager.Configure(pixfmt, nominalWidth, nominalHeight, maxWidth, maxHeight))
    m_bOpen = true;
  */

  return m_bOpen;
}

bool CRetroPlayerMemory::GetStreamBuffer(unsigned int width,
                                         unsigned int height,
                                         StreamBuffer& buffer)
{
  MemoryStreamBuffer& memoryBuffer = reinterpret_cast<MemoryStreamBuffer&>(buffer);

  if (m_bOpen)
  {
    //! @todo
  }

  return false;
}

void CRetroPlayerMemory::AddStreamData(const StreamPacket& packet)
{
  const MemoryStreamPacket& memoryPacket = reinterpret_cast<const MemoryStreamPacket&>(packet);

  if (m_bOpen)
  {
    //! @todo
  }
}

void CRetroPlayerMemory::CloseStream()
{
  if (m_bOpen)
  {
    CLog::Log(LOGDEBUG, "RetroPlayer[MEMORY]: Closing memory stream");
    m_bOpen = false;
  }
}
