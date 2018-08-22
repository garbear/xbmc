/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/buffers/BaseRenderBuffer.h"

#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
class CRenderContext;

class CRenderBufferFBO : public CBaseRenderBuffer
{
public:
  struct texture
  {
    GLuint tex_id;
    GLuint fbo_id;
    GLuint rbo_id;
  };

  CRenderBufferFBO(CRenderContext& context);
  ~CRenderBufferFBO() override = default;

  // implementation of IRenderBuffer via CRenderBufferSysMem
  bool UploadTexture() override;

  // implementation of IRenderBuffer
  bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) override;
  size_t GetFrameSize() const override { return 0; }
  uint8_t* GetMemory() override { return nullptr; }

  GLuint TextureID() const { return m_texture.tex_id; }
  uintptr_t GetCurrentFramebuffer() override;

  texture m_texture;

protected:
  CRenderContext& m_context;

  const GLenum m_textureTarget = GL_TEXTURE_2D; //! @todo

private:
  void DeleteTexture();
  bool CreateTexture();
  bool CreateFramebuffer();
  bool CreateDepthbuffer();
  bool CheckFrameBufferStatus();
};
} // namespace RETRO
} // namespace KODI
