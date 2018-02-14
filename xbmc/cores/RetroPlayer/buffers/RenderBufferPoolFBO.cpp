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

#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "utils/log.h"
#include "ServiceBroker.h"
#include "windowing/gbm/WinSystemGbmGLESContext.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferPoolFBO::CRenderBufferPoolFBO(CRenderContext &context) :
  m_context(context)
{
}

bool CRenderBufferPoolFBO::IsCompatible(const CRenderVideoSettings &renderSettings) const
{
  return true;
}

IRenderBuffer *CRenderBufferPoolFBO::CreateRenderBuffer(void *header /* = nullptr */)
{
  if(m_eglContext == EGL_NO_CONTEXT)
  {
    if (!CreateContext())
      return nullptr;
  }

  return new CRenderBufferFBO(m_context);
}

bool CRenderBufferPoolFBO::CreateContext()
{
  CWinSystemGbmGLESContext *winSystem = dynamic_cast<CWinSystemGbmGLESContext*>(&CServiceBroker::GetWinSystem());

  m_eglDisplay = winSystem->GetEGLDisplay();

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CLog::Log(LOGERROR, "failed to get EGL display");
    return false;
  }

  if (!eglInitialize(m_eglDisplay, NULL, NULL))
  {
    CLog::Log(LOGERROR, "failed to initialize EGL display");
    return false;
  }

  eglBindAPI(EGL_OPENGL_ES_API);

  EGLint attribs[] =
  {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
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

  // m_surface = gbm_surface_create(winSystem->GetGBMDevice(),
  //                                m_width,
  //                                m_height,
  //                                GBM_FORMAT_ARGB8888,
  //                                GBM_BO_USE_RENDERING);

  // if (!m_surface)
  // {
  //   CLog::Log(LOGERROR, "failed to create gbm surface");
  //   return false;
  // }

  // const EGLint window_attribs[] =
  // {
  //   EGL_RENDER_BUFFER, EGL_SINGLE_BUFFER, EGL_NONE
  // };

  // m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, (EGLNativeWindowType)m_surface, NULL); //window_attribs);
  // if (m_eglSurface == EGL_NO_SURFACE)
  // {
  //   CLog::Log(LOGERROR, "failed to create egl window surface");
  //   return false;
  // }

  int client_version = 2;

  const EGLint context_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, client_version, EGL_NONE
  };

  m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, context_attribs);
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