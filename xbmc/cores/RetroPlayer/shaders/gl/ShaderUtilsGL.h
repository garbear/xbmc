/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/ShaderTypes.h"

#include "system_gl.h"

namespace KODI
{
namespace SHADER
{

class CShaderUtilsGL
{
public:
  static GLint TranslateWrapType(WRAP_TYPE wrap);
  static void MoveVersionToFirstLine(std::string& source,
                                     std::string& defineVertex,
                                     std::string& defineFragment);
};

} // namespace SHADER
} // namespace KODI
