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

#include "RenderBufferFBO.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferFBO::CRenderBufferFBO(CRenderContext &context) :
  m_context(context)
{
}

bool CRenderBufferFBO::Allocate(AVPixelFormat format, unsigned int width, unsigned int height, size_t size)
{
  // Initialize IRenderBuffer
  m_format = format;
  m_width = width;
  m_height = height;

  if (!CreateTexture())
    return false;

  if (!CreateFramebuffer())
    return false;

  if (!CreateDepthbuffer())
    return false;

  return CheckFrameBufferStatus();
}

void CRenderBufferFBO::DeleteTexture()
{
  if (glIsTexture(m_texture.tex_id))
    glDeleteTextures(1, &m_texture.tex_id);

  m_texture.tex_id = 0;

  glDeleteFramebuffers(1, &m_texture.fbo_id);
  m_texture.fbo_id = 0;

  glDeleteRenderbuffers(1, &m_texture.rbo_id);
  m_texture.rbo_id = 0;
}

bool CRenderBufferFBO::UploadTexture()
{
  return true;
}

bool CRenderBufferFBO::CreateTexture()
{
  glGenTextures(1, &m_texture.tex_id);

  glBindTexture(m_textureTarget, m_texture.tex_id);
  glTexParameterf(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(m_textureTarget, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

  glBindTexture(m_textureTarget, 0);

  return true;
}

bool CRenderBufferFBO::CreateFramebuffer()
{
  glGenFramebuffers(1, &m_texture.fbo_id);
  glBindFramebuffer(GL_FRAMEBUFFER, m_texture.fbo_id);

  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D,
                         m_texture.tex_id,
                         0);

  return true;
}

bool CRenderBufferFBO::CreateDepthbuffer()
{
  glGenRenderbuffers(1, &m_texture.rbo_id);
  glBindRenderbuffer(GL_RENDERBUFFER, m_texture.rbo_id);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_width, m_height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_texture.rbo_id);

  return true;
}

bool CRenderBufferFBO::CheckFrameBufferStatus()
{
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if(status != GL_FRAMEBUFFER_COMPLETE)
    return false;

  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return true;
}
