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

#include "RPProcessInfoEGL.h"

#include <EGL/egl.h>

using namespace KODI;
using namespace RETRO;

CRPProcessInfoEGL::CRPProcessInfoEGL(std::string platformName) :
  CRPProcessInfo(std::move(platformName))
{
}

HwProcedureAddress CRPProcessInfoEGL::GetHwProcedureAddress(const char* symbol)
{
  return static_cast<HwProcedureAddress>(eglGetProcAddress(symbol));
}
