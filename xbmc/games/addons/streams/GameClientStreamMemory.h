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

#pragma once

#include "IGameClientStream.h"

struct game_stream_memory_properties;

namespace KODI
{
namespace RETRO
{
class IRetroPlayerStream;
struct MemoryStreamProperties;
} // namespace RETRO

namespace GAME
{

class CGameClientStreamMemory : public IGameClientStream
{
public:
  CGameClientStreamMemory() = default;
  ~CGameClientStreamMemory() override { CloseStream(); }

  // Implementation of IGameClientStream
  bool OpenStream(RETRO::IRetroPlayerStream* stream,
                  const game_stream_properties& properties) override;
  void CloseStream() override;
  bool GetBuffer(unsigned int width, unsigned int height, game_stream_buffer& buffer) override;
  void AddData(const game_stream_packet& packet) override;

private:
  // Utility functions
  static RETRO::MemoryStreamProperties* TranslateProperties(
      const game_stream_memory_properties& properties);

  // Stream parameters
  RETRO::IRetroPlayerStream* m_stream = nullptr;
};

} // namespace GAME
} // namespace KODI
