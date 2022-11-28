/*
 *      Copyright (C) 2017 Team Kodi
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

#include "cores/RetroPlayer/buffers/BaseRenderBuffer.h"

#include <memory>

#include "system_gl.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace KODI
{
namespace RETRO
{
class CRenderContext;

class CRenderBufferFBO : public CBaseRenderBuffer
{
public:
  CRenderBufferFBO(CRenderContext& context, uint32_t fboId);
  ~CRenderBufferFBO() override;

  // implementation of IRenderBuffer via CRenderBufferSysMem
  bool UploadTexture() override { return true; }

  // implementation of IRenderBuffer
  bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) override;
  size_t GetFrameSize() const override { return 0; }
  uint8_t* GetMemory() override { return nullptr; }

  uintptr_t GetCurrentFramebuffer() override;

  GLuint TextureID() const { return m_tex_id; }

protected:
  CRenderContext& m_context;

private:
  bool CreateTexture();
  bool CheckFrameBufferStatus();

  GLuint m_fbo_id;
  GLuint m_tex_id;
};
} // namespace RETRO
} // namespace KODI
