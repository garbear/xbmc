/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"

#include <gbm.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace KODI
{
namespace RETRO
{
class CRenderBufferFBO;
class IRenderBuffer;
class CRenderContext;

class CRenderBufferPoolFBO : public CBaseRenderBufferPool
{
public:
  CRenderBufferPoolFBO(CRenderContext& context);
  ~CRenderBufferPoolFBO() override = default;

  // implementation of IRenderBufferPool via CRenderBufferPoolSysMem
  bool IsCompatible(const CRenderVideoSettings& renderSettings) const override;

  // implementation of CBaseRenderBufferPool via CRenderBufferPoolSysMem
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;

protected:
  bool CreateContext();

  // Construction parameters
  CRenderContext& m_context;

  // Configuration parameters
  EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
  EGLConfig m_eglConfig{nullptr};
  EGLSurface m_eglSurface = EGL_NO_SURFACE;
  EGLContext m_eglContext = EGL_NO_CONTEXT;
  //gbm_surface *m_surface = nullptr;
};
} // namespace RETRO
} // namespace KODI
