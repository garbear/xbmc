/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/process/egl/RPProcessInfoEGL.h"

namespace KODI
{
namespace RETRO
{
// Based on the EGL process info so a hardware-rendering client can resolve GL
// functions. X11 runs on EGL unless it is asked for GLX, in which case there is
// no hardware rendering to be had and it is refused before a client relies on it.
class CRPProcessInfoX11 : public CRPProcessInfoEGL
{
public:
  CRPProcessInfoX11();

  static std::unique_ptr<CRPProcessInfo> Create();
  static void Register();
};
} // namespace RETRO
} // namespace KODI
