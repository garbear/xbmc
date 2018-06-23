/*
*  Copyright (C) 2018-2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
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
