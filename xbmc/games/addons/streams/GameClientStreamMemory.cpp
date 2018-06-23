/*
 *      Copyright (C) 2018 Team Kodi
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

#include "GameClientStreamMemory.h"

#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Game.h"
#include "cores/RetroPlayer/streams/RetroPlayerMemory.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

bool CGameClientStreamMemory::OpenStream(RETRO::IRetroPlayerStream* stream,
                                         const game_stream_properties& properties)
{
  RETRO::CRetroPlayerMemory* memoryStream = dynamic_cast<RETRO::CRetroPlayerMemory*>(stream);
  if (memoryStream == nullptr)
  {
    CLog::Log(LOGERROR, "GAME: RetroPlayer stream is not a memory stream");
    return false;
  }

  std::unique_ptr<RETRO::MemoryStreamProperties> memoryProperties(
      TranslateProperties(properties.memory));
  if (memoryProperties)
  {
    if (memoryStream->OpenStream(
            reinterpret_cast<const RETRO::StreamProperties&>(*memoryProperties)))
      m_stream = stream;
  }

  return m_stream != nullptr;
}

void CGameClientStreamMemory::CloseStream()
{
  if (m_stream != nullptr)
  {
    m_stream->CloseStream();
    m_stream = nullptr;
  }
}

bool CGameClientStreamMemory::GetBuffer(unsigned int width,
                                        unsigned int height,
                                        game_stream_buffer& buffer)
{
  if (m_stream != nullptr)
  {
    RETRO::MemoryStreamBuffer streamBuffer;
    if (m_stream->GetStreamBuffer(width, height,
                                  reinterpret_cast<RETRO::StreamBuffer&>(streamBuffer)))
    {
      buffer.type = GAME_STREAM_MEMORY;

      game_stream_memory_buffer& memory = buffer.memory;

      memory.data = streamBuffer.data;
      memory.size = streamBuffer.size;

      return true;
    }
  }

  return false;
}

void CGameClientStreamMemory::AddData(const game_stream_packet& packet)
{
  if (packet.type != GAME_STREAM_MEMORY)
    return;

  if (m_stream != nullptr)
  {
    const game_stream_memory_packet& memory = packet.memory;

    RETRO::MemoryStreamPacket memoryPacket{
        memory.data,
        memory.size,
    };

    m_stream->AddStreamData(reinterpret_cast<const RETRO::StreamPacket&>(memoryPacket));
  }
}

RETRO::MemoryStreamProperties* CGameClientStreamMemory::TranslateProperties(
    const game_stream_memory_properties& properties)
{
  return new RETRO::MemoryStreamProperties{properties.size};
}
