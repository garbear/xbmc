/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderUtilsGL.h"

#include "ServiceBroker.h"
#ifndef HAS_GLES
#include "rendering/gl/RenderSystemGL.h"
#else
#include "rendering/gles/RenderSystemGLES.h"
#endif

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
#ifndef HAS_GLES
      glWrap = GL_CLAMP_TO_BORDER;
#else
      glWrap = GL_CLAMP_TO_EDGE;
#endif
      break;
  }
  return glWrap;
}

std::string CShaderUtilsGL::GetGLSLVersion(std::string& source)
{
  unsigned int version;
  std::string versionString;

  size_t sourceVersionPosition = source.find("#version ");

  // Extract the version from the source
  if (sourceVersionPosition != std::string::npos)
  {
    size_t sourceVersionSize;
    std::string sourceVersion = source.substr(sourceVersionPosition + 9);

    version = std::stoul(sourceVersion, &sourceVersionSize, 10);

    // Keep only source code after the version define
    source = sourceVersion.substr(sourceVersionSize);

#ifndef HAS_GLES
    versionString = "#version " + std::to_string(version) + "\n";
#else
    if (version >= 130 && version < 330)
      versionString = "300 es";
    else if (version == 330)
      versionString = "310 es";
    else if (version > 330)
      versionString = "320 es";
    else
      versionString = "100";

    versionString = "#version " + versionString + "\n";
#endif
  }
  // Set GLSL version according to GL version
  else
  {
    unsigned int major, minor;
    CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
    version = major * 100 + minor * 10;

#ifndef HAS_GLES
    if (version == 200)
      versionString = "110";
    else if (version == 210)
      versionString = "120";
    else if (version == 300)
      versionString = "130";
    else if (version == 310)
      versionString = "140";
    else if (version == 320)
      versionString = "150";
    else
      versionString = std::to_string(version);

    versionString = "#version " + versionString + "\n";
#else
    if (version == 200)
      versionString = "100";
    else
      versionString = std::to_string(version) + " es";

    // Do not force GLSL version for OpenGL ES
    versionString = "\n";
#endif
  }
  return versionString;
}
