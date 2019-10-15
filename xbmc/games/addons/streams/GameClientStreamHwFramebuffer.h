/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IGameClientStream.h"

namespace KODI
{
namespace GAME
{

class CGameClientStreamHwFramebuffer : public IGameClientStream
{
public:
  CGameClientStreamHwFramebuffer() = default;
  ~CGameClientStreamHwFramebuffer() override = default;

  // Implementation of IGameClientStream
  bool OpenStream(RETRO::IRetroPlayerStream* stream,
                  const game_stream_properties& properties) override;
  void CloseStream() override;
  bool GetBuffer(unsigned int width, unsigned int height, game_stream_buffer& buffer) override;
  void AddData(const game_stream_packet& packet) override;

protected:
  // Stream parameters
  RETRO::IRetroPlayerStream* m_stream = nullptr;
};

} // namespace GAME
} // namespace KODI
