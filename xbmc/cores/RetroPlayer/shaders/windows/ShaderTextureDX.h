/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderTexture.h"
#include "guilib/D3DResource.h"
#include "guilib/Texture.h"

#include <d3d11.h>

namespace KODI
{

namespace SHADER
{

template<typename TextureType>
class CShaderTextureDX : public IShaderTexture
{
public:
  CShaderTextureDX() = default;
  CShaderTextureDX(TextureType* texture) : m_texture(texture) {}
  CShaderTextureDX(TextureType& texture) : m_texture(&texture) {}

  // Destructor
  // Don't delete texture since it wasn't created here
  ~CShaderTextureDX() override = default;

  float GetWidth() const override { return static_cast<float>(m_texture->GetWidth()); }
  float GetHeight() const override { return static_cast<float>(m_texture->GetHeight()); }

  void SetTexture(TextureType* newTexture) { m_texture = newTexture; }

  ID3D11ShaderResourceView *GetShaderResource() const { return m_texture->GetShaderResource(); }

  TextureType* GetPointer() { return m_texture; }

private:
  TextureType* m_texture = nullptr;
};

using CShaderTextureCD3D = CShaderTextureDX<CD3DTexture>;
using CShaderTextureCDX = CShaderTextureDX<CDXTexture>;

}
}
