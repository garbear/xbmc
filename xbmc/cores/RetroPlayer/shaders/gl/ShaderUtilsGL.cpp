/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderUtilsGL.h"

#include <fstream>
#include <sstream>

using namespace KODI;
using namespace SHADER;

GLint CShaderUtilsGL::TranslateWrapType(WRAP_TYPE wrap)
{
  GLint glWrap;
  switch (wrap)
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
      glWrap = GL_CLAMP_TO_BORDER;
      break;
  }

  return glWrap;
}

void CShaderUtilsGL::MoveVersionToFirstLine(std::string& source,
                                            std::string& defineVertex,
                                            std::string& defineFragment)
{
  std::istringstream str_stream(source);
  source.clear();

  std::string line;
  bool firstLine = true;
  while (std::getline(str_stream, line))
  {
    if (!firstLine)
    {
      source += line;
    }
    else
    {
      defineVertex = line + "\n" + defineVertex;
      defineFragment = line + "\n" + defineFragment;
      firstLine = false;
    }
  }
}
