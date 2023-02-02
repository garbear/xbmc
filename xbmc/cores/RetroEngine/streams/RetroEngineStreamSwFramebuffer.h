/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRetroEngineStream.h"

namespace KODI
{
namespace RETRO
{
class IRetroPlayerStream;
}

namespace RETRO_ENGINE
{

class CRetroEngineStreamSwFramebuffer : public IRetroEngineStream
{
public:
  CRetroEngineStreamSwFramebuffer() = default;
  ~CRetroEngineStreamSwFramebuffer() override { CloseStream(); }

  // Implementation of IRetroEngineStream
  bool OpenStream(RETRO::IRetroPlayerStream* stream,
                  AVPixelFormat nominalFormat,
                  unsigned int nominalWidth,
                  unsigned int nominalHeight) override;
  void CloseStream() override;
  bool GetBuffer(unsigned int width,
                 unsigned int height,
                 AVPixelFormat& format,
                 uint8_t*& data,
                 size_t& size) override;
  void ReleaseBuffer(uint8_t* data) override {}
  void AddData(unsigned int width, unsigned int height, const uint8_t* data, size_t size) override;

protected:
  // Stream parameters
  RETRO::IRetroPlayerStream* m_stream = nullptr;
};

} // namespace RETRO_ENGINE
} // namespace KODI
