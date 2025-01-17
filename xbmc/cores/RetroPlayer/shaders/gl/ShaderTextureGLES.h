/*
 *  Copyright (C) 2019-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderTexture.h"
#include "guilib/Texture.h"
#include "guilib/TextureGLES.h"

namespace KODI
{
namespace SHADER
{

class CShaderTextureGLES : public IShaderTexture
{
public:
  CShaderTextureGLES() = default;
  CShaderTextureGLES(CGLESTexture* texture, GLint internalFormat = 0) : m_texture(texture), m_internalFormat(internalFormat) {}
  CShaderTextureGLES(CGLESTexture& texture, GLint internalFormat = 0) : m_texture(&texture), m_internalFormat(internalFormat) {}

  // Destructor
  ~CShaderTextureGLES() override;

  float GetWidth() const override { return static_cast<float>(m_texture->GetWidth()); }
  float GetHeight() const override { return static_cast<float>(m_texture->GetHeight()); }

  CGLESTexture* GetPointer() { return m_texture; }

  bool CreateFBO();
  bool BindFBO();
  void UnbindFBO();

private:
  CGLESTexture* m_texture = nullptr;
  GLint m_internalFormat = 0;
  GLuint FBO = 0;
};
} // namespace SHADER
} // namespace KODI
