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

#include "RenderBufferPoolFBO.h"

#include "RenderBufferFBO.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/linux/WinSystemEGL.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferPoolFBO::CRenderBufferPoolFBO(CRenderContext& context) : m_context(context)
{
}

CRenderBufferPoolFBO::~CRenderBufferPoolFBO()
{
  glDeleteFramebuffers(1, &m_fbo_id);
  m_fbo_id = 0;

  //! @todo: this needs to be done from the gameloop thread
  // eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  // eglDestroyContext(m_eglDisplay, m_eglContext);
}

bool CRenderBufferPoolFBO::IsCompatible(const CRenderVideoSettings& renderSettings) const
{
  return true;
}

IRenderBuffer* CRenderBufferPoolFBO::CreateRenderBuffer(void* header /* = nullptr */)
{
  if (m_eglContext == EGL_NO_CONTEXT)
  {
    if (!CreateContext())
      return nullptr;
  }

  if (m_fbo_id == 0)
  {
    if (!CreateFramebuffer())
      return nullptr;
  }

  return new CRenderBufferFBO(m_context, m_fbo_id);
}

bool CRenderBufferPoolFBO::CreateContext()
{
  auto winSystem =
      dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(CServiceBroker::GetWinSystem());

  m_eglDisplay = winSystem->GetEGLDisplay();

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CLog::Log(LOGERROR, "failed to get EGL display");
    return false;
  }

  if (!eglInitialize(m_eglDisplay, nullptr, nullptr))
  {
    CLog::Log(LOGERROR, "failed to initialize EGL display");
    return false;
  }

// clang-format off

//! @todo: improve this
#if defined (HAS_GLES)
  eglBindAPI(EGL_OPENGL_ES_API);
#elif defined (HAS_GL)
  eglBindAPI(EGL_OPENGL_API);
#endif

  EGLint attribs[] =
  {
#if defined (HAS_GLES)
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#elif defined (HAS_GL)
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_NONE
  };

  EGLint neglconfigs;
  if (!eglChooseConfig(m_eglDisplay, attribs, &m_eglConfig, 1, &neglconfigs))
  {
    CLog::Log(LOGERROR, "Failed to query number of EGL configs");
    return false;
  }

  if (neglconfigs <= 0)
  {
    CLog::Log(LOGERROR, "No suitable EGL configs found");
    return false;
  }

#if defined (HAS_GLES)
  const EGLint context_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
#elif defined (HAS_GL)
  const EGLint context_attribs[] = {
    EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
    EGL_CONTEXT_MINOR_VERSION_KHR, 2,
    EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
    EGL_NONE
  };
#endif

  // clang-format on

  m_eglContext =
      eglCreateContext(m_eglDisplay, m_eglConfig, winSystem->GetEGLContext(), context_attribs);
  if (m_eglContext == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR, "failed to create EGL context");
    return false;
  }

  if (!eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext))
  {
    CLog::Log(LOGERROR, "Failed to make context current");
    return false;
  }

  return true;
}

bool CRenderBufferPoolFBO::CreateFramebuffer()
{
  glGenFramebuffers(1, &m_fbo_id);

  return true;
}

void CRenderBufferPoolFBO::DestroyContext()
{
  CLog::Log(LOGDEBUG, "DestroyContext");

  eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(m_eglDisplay, m_eglContext);
}
