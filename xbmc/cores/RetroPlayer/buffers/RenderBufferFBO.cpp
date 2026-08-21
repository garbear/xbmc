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

#include "ServiceBroker.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/linux/WinSystemEGL.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferFBO::CRenderBufferFBO(CRenderContext& context,
                                   bool depth,
                                   bool stencil,
                                   bool bottomLeftOrigin)
  : m_context(context),
    m_depth(depth),
    m_stencil(stencil),
    m_bottomLeftOrigin(bottomLeftOrigin)
{
}

CRenderBufferFBO::~CRenderBufferFBO()
{
  if (m_fbo_id != 0)
  {
    glDeleteFramebuffers(1, &m_fbo_id);
    m_fbo_id = 0;
  }

  if (m_tex_id != 0)
  {
    glDeleteTextures(1, &m_tex_id);
    m_tex_id = 0;
  }

  if (m_depth_stencil_id != 0)
  {
    glDeleteRenderbuffers(1, &m_depth_stencil_id);
    m_depth_stencil_id = 0;
  }

}


bool CRenderBufferFBO::Allocate(AVPixelFormat format, unsigned int width, unsigned int height)
{
  // Initialize IRenderBuffer
  m_format = format;
  m_width = width;
  m_height = height;

  // The texture stays this size for the buffer's life, while the reported width
  // and height follow whatever the client draws into it each frame
  m_textureWidth = width;
  m_textureHeight = height;

  if (!CreateTexture())
    return false;

  if (!CreateDepthStencil())
    return false;

  // Each buffer owns its framebuffer, so its attachments are made once here
  // rather than rebuilt and revalidated on every frame, and several buffers
  // can be in flight without fighting over one framebuffer's attachments.
  glGenFramebuffers(1, &m_fbo_id);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex_id, 0);

  if (m_depth_stencil_id != 0)
  {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              m_depth_stencil_id);

    if (m_stencil)
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                m_depth_stencil_id);
  }

  const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Framebuffer is incomplete, status {:#x}", status);
    return false;
  }

  return true;
}

bool CRenderBufferFBO::CreateDepthStencil()
{
  // Stencil without depth is not a valid request and is ignored, per the Game
  // API contract
  if (!m_depth)
    return true;

  glGenRenderbuffers(1, &m_depth_stencil_id);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depth_stencil_id);

  // A packed 24/8 buffer when both were asked for, depth alone otherwise
  glRenderbufferStorage(GL_RENDERBUFFER, m_stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24,
                        m_width, m_height);

  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Attached {} buffer to the client's framebuffer",
            m_stencil ? "packed depth/stencil" : "depth");

  return true;
}

bool CRenderBufferFBO::CreateTexture()
{
  glBindTexture(GL_TEXTURE_2D, 0);
  glGenTextures(1, &m_tex_id);

  glBindTexture(GL_TEXTURE_2D, m_tex_id);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Only level 0 is ever allocated, and the core redraws it every frame, so a
  // mipmapped min filter would leave the texture incomplete and sample black.
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

  return true;
}

uintptr_t CRenderBufferFBO::GetCurrentFramebuffer()
{
  // Attachments were made and validated in Allocate(). This is called for every
  // frame the client renders, so it must stay free of driver round trips.
  return m_fbo_id;
}
