/*
*  Copyright (C) 2018-2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#pragma once

#include "IRetroPlayerStream.h"

#include <stddef.h>
#include <stdint.h>

namespace KODI
{
namespace RETRO
{
struct MemoryStreamProperties
{
  MemoryStreamProperties(size_t size) : size(size) {}

  size_t size;
};

struct MemoryStreamBuffer
{
  MemoryStreamBuffer() = default;

  MemoryStreamBuffer(uint8_t* data, size_t size) : data(data), size(size) {}

  uint8_t* data;
  size_t size;
};

struct MemoryStreamPacket
{
  MemoryStreamPacket(const uint8_t* data, size_t size) : data(data), size(size) {}

  const uint8_t* data;
  size_t size;
};

class CRetroPlayerMemory : public IRetroPlayerStream
{
public:
  explicit CRetroPlayerMemory();
  ~CRetroPlayerMemory() override;

  // implementation of IRetroPlayerStream
  bool OpenStream(const StreamProperties& properties) override;
  bool GetStreamBuffer(unsigned int width, unsigned int height, StreamBuffer& buffer) override;
  void AddStreamData(const StreamPacket& packet) override;
  void CloseStream() override;

private:
  // Stream properties
  bool m_bOpen{false};
};
} // namespace RETRO
} // namespace KODI
