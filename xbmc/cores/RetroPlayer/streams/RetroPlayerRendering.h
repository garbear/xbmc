/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRetroPlayerStream.h"
#include "RetroPlayerStreamTypes.h"

#include <stdint.h>

namespace KODI
{
namespace RETRO
{
class CRPProcessInfo;
class CRPRenderManager;

struct HwFramebufferBuffer : public StreamBuffer
{
  uintptr_t framebuffer;
};

struct HwFramebufferPacket : public StreamPacket
{
  HwFramebufferPacket(uintptr_t framebuffer) : framebuffer(framebuffer) {}

  uintptr_t framebuffer;
};

class CRetroPlayerRendering : public IRetroPlayerStream
{
public:
  CRetroPlayerRendering(CRPRenderManager& m_renderManager, CRPProcessInfo& m_processInfo);

  ~CRetroPlayerRendering() override = default;

  // implementation of IRetroPlayerStream
  bool OpenStream(const StreamProperties& properties) override;
  bool GetStreamBuffer(unsigned int width, unsigned int height, StreamBuffer& buffer) override;
  void AddStreamData(const StreamPacket& packet) override;
  void CloseStream() override;

private:
  // Construction parameters
  CRPRenderManager& m_renderManager;
  CRPProcessInfo& m_processInfo;

  // Rendering properties
  unsigned int m_width;
  unsigned int m_height;
};
} // namespace RETRO
} // namespace KODI
