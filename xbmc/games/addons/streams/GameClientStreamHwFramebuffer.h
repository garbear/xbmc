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

namespace KODI
{
namespace RETRO
{
  class IRetroPlayerStream;
}

namespace GAME
{

class IHwFramebufferCallback
{
public:
  virtual ~IHwFramebufferCallback() = default;

  /*!
   * \brief Invalidates the current HW context and reinitializes GPU resources
   *
   * Any GL state is lost, and must not be deinitialized explicitly.
   */
  virtual void HardwareContextReset() = 0;
};

class CGameClientStreamHwFramebuffer : public IGameClientStream
{
public:
  CGameClientStreamHwFramebuffer(IHwFramebufferCallback& callback);
  ~CGameClientStreamHwFramebuffer() override = default;

  // Implementation of IGameClientStream
  bool OpenStream(RETRO::IRetroPlayerStream* stream, const game_stream_properties& properties) override;
  void CloseStream() override;
  bool GetBuffer(unsigned int width, unsigned int height, game_stream_buffer& buffer);
  void AddData(const game_stream_packet& packet) override;

private:
  // Construction parameters
  IHwFramebufferCallback& m_callback;

  // Stream parameters
  RETRO::IRetroPlayerStream* m_stream;
};

} // namespace GAME
} // namespace KODI
