/*
*  Copyright (C) 2018-2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
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
    (void)memoryBuffer;
  }

  return false;
}

void CRetroPlayerMemory::AddStreamData(const StreamPacket& packet)
{
  const MemoryStreamPacket& memoryPacket = reinterpret_cast<const MemoryStreamPacket&>(packet);

  if (m_bOpen)
  {
    //! @todo
    (void)memoryPacket;
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
