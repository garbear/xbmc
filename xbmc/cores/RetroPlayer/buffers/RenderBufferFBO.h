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

    CRenderBufferFBO(CRenderContext &context);
    ~CRenderBufferFBO() override = default;

    // implementation of IRenderBuffer via CRenderBufferSysMem
    bool UploadTexture() override;

    // implementation of IRenderBuffer
    bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height, size_t size) override;
    size_t GetFrameSize() const override { return 0; }
    uint8_t *GetMemory() override { return nullptr; }

    GLuint TextureID() const { return m_texture.tex_id; }
    uintptr_t GetCurrentFramebuffer() override { return m_texture.fbo_id; }

    texture m_texture;

  protected:
    CRenderContext &m_context;

    const GLenum m_textureTarget = GL_TEXTURE_2D; //! @todo

  private:
    void DeleteTexture();
    bool CreateTexture();
    bool CreateFramebuffer();
    bool CreateDepthbuffer();
    bool CheckFrameBufferStatus();
  };
}
}
