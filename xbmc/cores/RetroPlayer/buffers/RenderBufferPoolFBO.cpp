/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferPoolFBO.h"

#include "RenderBufferFBO.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "utils/log.h"
#include "windowing/gbm/WinSystemGbmEGLContext.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferPoolFBO::CRenderBufferPoolFBO(CRenderContext& context) : m_context(context)
{
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

  return new CRenderBufferFBO(m_context);
}

bool CRenderBufferPoolFBO::CreateContext()
{
  WINDOWING::GBM::CWinSystemGbmEGLContext* winSystem =
      dynamic_cast<WINDOWING::GBM::CWinSystemGbmEGLContext*>(CServiceBroker::GetWinSystem());

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

  eglBindAPI(EGL_OPENGL_API);

  EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                      EGL_OPENGL_BIT,
                      EGL_SURFACE_TYPE,
                      EGL_WINDOW_BIT,
                      EGL_RED_SIZE,
                      8,
                      EGL_GREEN_SIZE,
                      8,
                      EGL_BLUE_SIZE,
                      8,
                      EGL_ALPHA_SIZE,
                      8,
                      EGL_DEPTH_SIZE,
                      16,
                      EGL_NONE};

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

  const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, client_version, EGL_NONE};

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
