/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderTexture.h"
#include "guilib/Texture.h"
#include "guilib/TextureGL.h"

namespace KODI
{
namespace SHADER
{

class CShaderTextureGL : public IShaderTexture
{
public:
  CShaderTextureGL() = default;
  CShaderTextureGL(CGLTexture* texture, GLint internalFormat = 0) : m_texture(texture), m_internalFormat(internalFormat) {}
  CShaderTextureGL(CGLTexture& texture, GLint internalFormat = 0) : m_texture(&texture), m_internalFormat(internalFormat) {}

  // Destructor
  ~CShaderTextureGL() override;

  float GetWidth() const override { return static_cast<float>(m_texture->GetWidth()); }
  float GetHeight() const override { return static_cast<float>(m_texture->GetHeight()); }

  CGLTexture* GetPointer() { return m_texture; }

  bool CreateFBO();
  bool BindFBO();
  void UnbindFBO();

private:
  CGLTexture* m_texture = nullptr;
  GLint m_internalFormat = 0;
  GLuint FBO = 0;
};
} // namespace SHADER
} // namespace KODI
