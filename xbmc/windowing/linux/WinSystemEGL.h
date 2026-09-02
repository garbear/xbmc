/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/EGLUtils.h"

namespace KODI
{
namespace WINDOWING
{
namespace LINUX
{

class CWinSystemEGL
{
public:
  CWinSystemEGL(EGLenum platform, std::string const& platformExtension);
  virtual ~CWinSystemEGL() = default;

  // Virtual so a window system that keeps its EGL handles somewhere other than
  // the context below can still be found by a dynamic_cast to this class and
  // asked for them
  virtual EGLDisplay GetEGLDisplay() const;
  virtual EGLSurface GetEGLSurface() const;
  virtual EGLContext GetEGLContext() const;
  virtual EGLConfig GetEGLConfig() const;

protected:
  CEGLContextUtils m_eglContext;
};

} // namespace LINUX
} // namespace WINDOWING
} // namespace KODI
