/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderLutGLES.h"

#include "ShaderTextureGLES.h"
#include "ShaderUtilsGLES.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/IShaderPreset.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"

#include <utility>

using namespace KODI;
using namespace SHADER;

CShaderLutGLES::CShaderLutGLES(const std::string& id, const std::string& path)
  : IShaderLut(id, path)
{
}

CShaderLutGLES::~CShaderLutGLES() = default;

bool CShaderLutGLES::Create(RETRO::CRenderContext& context, const ShaderLut& lut)
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

std::unique_ptr<IShaderTexture> CShaderLutGLES::CreateLUTTexture(RETRO::CRenderContext& context,
                                                                 const ShaderLut& lut)
{
  std::unique_ptr<CTexture> texture = CTexture::LoadFromFile(lut.path);
  CGLESTexture* textureGL = static_cast<CGLESTexture*>(texture.get());

  if (textureGL == nullptr)
  {
    CLog::Log(LOGERROR, "Couldn't open LUT {}", lut.path);
    return std::unique_ptr<IShaderTexture>();
  }

  if (lut.mipmap)
    textureGL->SetMipmapping();

  textureGL->SetScalingMethod(lut.filterType == FilterType::LINEAR ? TEXTURE_SCALING::LINEAR
                                                                   : TEXTURE_SCALING::NEAREST);
  textureGL->LoadToGPU();

  const GLint wrapType = CShaderUtilsGLES::TranslateWrapType(lut.wrapType);

  glBindTexture(GL_TEXTURE_2D, textureGL->getMTexture());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapType);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapType);

  return std::unique_ptr<IShaderTexture>(
      new CShaderTextureGLES(static_cast<CGLESTexture*>(texture.release())));
}
