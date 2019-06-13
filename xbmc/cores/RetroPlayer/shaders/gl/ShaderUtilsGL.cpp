/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <fstream>
#include <sstream>
#include "ShaderUtilsGL.h"

using namespace KODI;
using namespace SHADER;

GLint CShaderUtilsGL::TranslateWrapType(WRAP_TYPE wrap)
{
GLint glWrap;
switch(wrap)
{
  case WRAP_TYPE_EDGE:
    glWrap = GL_CLAMP_TO_EDGE;
    break;
  case WRAP_TYPE_REPEAT:
    glWrap = GL_REPEAT;
    break;
  case WRAP_TYPE_MIRRORED_REPEAT:
    glWrap = GL_MIRRORED_REPEAT;
    break;
  case WRAP_TYPE_BORDER:
  default:
    glWrap = GL_CONSTANT_BORDER;
  }
  return glWrap;
}

std::string CShaderUtilsGL::FileToString(std::string source)
{
  std::ifstream t(source);
  std::stringstream target;
  target << t.rdbuf();
  return target.str();
}
