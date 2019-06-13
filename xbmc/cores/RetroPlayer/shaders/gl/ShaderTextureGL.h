/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderTexture.h"
#include "guilib/TextureGL.h"
#include "guilib/Texture.h"

namespace KODI
{
namespace SHADER
{
class CShaderTextureGL : public IShaderTexture
{
public:
  CShaderTextureGL() = default;

  CShaderTextureGL(CGLTexture *texture) : m_texture(texture) {}
  CShaderTextureGL(CGLTexture &texture) : m_texture(&texture) {}

  // Destructor
  // Don't delete texture since it wasn't created here
  ~CShaderTextureGL() override = default;

  float GetWidth() const override { return static_cast<float>(m_texture->GetWidth()); }
  float GetHeight() const override { return static_cast<float>(m_texture->GetHeight()); }

  void SetTexture(CGLTexture *newTexture) { m_texture = newTexture; }
  CGLTexture *GetPointer() { return m_texture; }

private:
  CGLTexture *m_texture = nullptr;
};
}
}
