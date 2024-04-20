/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderTextureGL.h"

#include "utils/log.h"

using namespace KODI;
using namespace SHADER;

bool CShaderTextureGL::CreateFBO()
{
  if (FBO == 0)
    glGenFramebuffers(1, &FBO);

  return true;
}

bool CShaderTextureGL::BindFBO()
{
  GLuint renderTargetID = GetPointer()->getMTexture();
  if (renderTargetID == 0)
    return false;

  if (FBO == 0)
    if (!CreateFBO())
      return false;

  glBindFramebuffer(GL_FRAMEBUFFER, FBO);
  glBindTexture(GL_TEXTURE_2D, renderTargetID);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTargetID, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    CLog::Log(LOGERROR, "{}: Framebuffer is not complete!", __func__);
    UnbindFBO();
    return false;
  }

  return true;
}

void CShaderTextureGL::UnbindFBO()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

CShaderTextureGL::~CShaderTextureGL()
{
  if (FBO != 0)
    glDeleteFramebuffers(1, &FBO);
}
