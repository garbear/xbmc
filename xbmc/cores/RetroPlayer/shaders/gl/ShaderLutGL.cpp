/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderLutGL.h"

#ifndef HAS_GLES
#include "ShaderTextureGL.h"
#else
#include "ShaderTextureGLES.h"
#endif
#include "ShaderUtilsGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/IShaderPreset.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"

#include <utility>
#define MAX_FLOAT 3.402823466E+38

using namespace KODI;
using namespace SHADER;

CShaderLutGL::CShaderLutGL(const std::string& id, const std::string& path) : IShaderLut(id, path)
{
}

CShaderLutGL::~CShaderLutGL() = default;

bool CShaderLutGL::Create(RETRO::CRenderContext& context, const ShaderLut& lut)
{
  std::unique_ptr<IShaderTexture> lutTexture(CreateLUTTexture(context, lut));
  if (!lutTexture)
  {
    CLog::Log(LOGWARNING, "{} - Couldn't create a LUT texture for LUT {}", __FUNCTION__, lut.strId);
    return false;
  }

  m_texture = std::move(lutTexture);

  return true;
}

std::unique_ptr<IShaderTexture> CShaderLutGL::CreateLUTTexture(RETRO::CRenderContext& context,
                                                               const KODI::SHADER::ShaderLut& lut)
{
  std::unique_ptr<CTexture> texture = CTexture::LoadFromFile(lut.path);
#ifndef HAS_GLES
  CGLTexture* textureGL = static_cast<CGLTexture*>(texture.get());
#else
  CGLESTexture* textureGL = static_cast<CGLESTexture*>(texture.get());
#endif

  if (textureGL == nullptr)
  {
    CLog::Log(LOGERROR, "Couldn't open LUT {}", lut.path);
    return std::unique_ptr<IShaderTexture>();
  }

  if (lut.mipmap)
    textureGL->SetMipmapping();

  textureGL->SetScalingMethod(lut.filter == FILTER_TYPE_LINEAR ? TEXTURE_SCALING::LINEAR : TEXTURE_SCALING::NEAREST);
  textureGL->LoadToGPU();

  auto wrapType = CShaderUtilsGL::TranslateWrapType(lut.wrap);

  glBindTexture(GL_TEXTURE_2D, textureGL->getMTexture());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapType);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapType);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapType);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0.0);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, MAX_FLOAT);

#ifndef HAS_GLES
  GLfloat blackBorder[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, blackBorder);
#endif

#ifndef HAS_GLES
  return std::unique_ptr<IShaderTexture>(
      new CShaderTextureGL(static_cast<CGLTexture*>(texture.release())));
#else
  return std::unique_ptr<IShaderTexture>(
      new CShaderTextureGLES(static_cast<CGLESTexture*>(texture.release())));
#endif
}
