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

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
class CRenderContext;

class CRenderBufferFBO : public CBaseRenderBuffer
{
public:
  CRenderBufferFBO(CRenderContext& context, bool depth, bool stencil, bool bottomLeftOrigin);
  ~CRenderBufferFBO() override;

  // implementation of IRenderBuffer via CRenderBufferSysMem
  bool UploadTexture() override { return true; }

  // implementation of IRenderBuffer
  bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) override;
  size_t GetFrameSize() const override { return 0; }
  uint8_t* GetMemory() override { return nullptr; }

  uintptr_t GetCurrentFramebuffer() override;

  GLuint TextureID() const { return m_tex_id; }

  /*!
   * \brief Size of the texture backing this buffer
   *
   * The buffer reports the size of the frame the client drew, which is usually
   * smaller than the texture holding it, so these are what texture coordinates
   * have to be measured against.
   */
  unsigned int TextureWidth() const { return m_textureWidth; }
  unsigned int TextureHeight() const { return m_textureHeight; }

  //! \brief True if the client rendered with OpenGL's bottom-left origin
  bool BottomLeftOrigin() const { return m_bottomLeftOrigin; }


protected:
  CRenderContext& m_context;

private:
  bool CreateTexture();
  bool CreateDepthStencil();
  bool CheckFrameBufferStatus();

  GLuint m_fbo_id{0};
  GLuint m_tex_id{0};
  unsigned int m_textureWidth{0};
  unsigned int m_textureHeight{0};

  // Depth and stencil live in a renderbuffer rather than a texture: the client
  // draws into them but nothing samples them afterwards.
  const bool m_depth;
  const bool m_stencil;
  const bool m_bottomLeftOrigin;
  GLuint m_depth_stencil_id{0};

};
} // namespace RETRO
} // namespace KODI
