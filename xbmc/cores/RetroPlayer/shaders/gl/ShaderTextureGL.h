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

  CShaderTextureGL(CGLTexture* texture) : m_texture(texture) {}
  CShaderTextureGL(CGLTexture& texture) : m_texture(&texture) {}

  // Destructor
  // Don't delete texture since it wasn't created here
  ~CShaderTextureGL() override;

  float GetWidth() const override { return static_cast<float>(m_texture->GetWidth()); }
  float GetHeight() const override { return static_cast<float>(m_texture->GetHeight()); }

  CGLTexture* GetPointer() { return m_texture; }
  bool CreateFBO(int width, int height);
  void BindFBO();
  void UnbindFBO();

private:
  CGLTexture* m_texture = nullptr;
  GLuint FBO = 0;
};
} // namespace SHADER
} // namespace KODI
