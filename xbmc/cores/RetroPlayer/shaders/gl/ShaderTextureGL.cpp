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

bool CShaderTextureGL::CreateFBO(int width, int height)
{
  if (FBO == 0)
    glGenFramebuffers(1, &FBO);

  GLuint renderTargetID = GetPointer()->getMTexture();
  if (renderTargetID == 0)
    return false;

  BindFBO();
  glBindTexture(GL_TEXTURE_2D, renderTargetID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  width,  height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTargetID, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    CLog::Log(LOGERROR, "%s: Framebuffer is not complete!", __func__);
    UnbindFBO();
    return false;
  }
  UnbindFBO();
  return true;
}

void CShaderTextureGL::BindFBO()
{
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);
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
