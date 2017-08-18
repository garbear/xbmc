#pragma once
/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>
#include <string>

#include "cores/RetroPlayer/rendering/VideoShaders/IVideoShaderTexture.h"
#include "guilib/Texture.h"

#if !defined(SAFE_RELEASE)
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }
#endif

namespace KODI
{
namespace RETRO
{
  class CRenderContext;
}

namespace SHADER
{

class CShaderSamplerDX : public IShaderSampler
{
public:
  CShaderSamplerDX(ID3D11SamplerState* sampler)
    : m_sampler(sampler)
  {
  }

  ~CShaderSamplerDX() override
  {
    SAFE_RELEASE(m_sampler);
  }

private:
  ID3D11SamplerState* m_sampler;
};

class CShaderTextureCD3D : public IShaderTexture
{
  using TextureType = CD3DTexture;
public:
  CShaderTextureCD3D() = default;
  CShaderTextureCD3D(TextureType* texture) : m_texture(texture) {}
  CShaderTextureCD3D(TextureType& texture) : m_texture(&texture) {}

  // Destructor
  // Don't delete texture since it wasn't created here
  ~CShaderTextureCD3D() override = default;

  float GetWidth() const override { return static_cast<float>(m_texture->GetWidth()); }
  float GetHeight() const override { return static_cast<float>(m_texture->GetHeight()); }

  void SetTexture(TextureType* newTexture) { m_texture = newTexture; }

  ID3D11ShaderResourceView *GetShaderResource() const { return m_texture->GetShaderResource(); }

  TextureType* GetPointer() { return m_texture; }

private:
  TextureType* m_texture = nullptr;
};

class CShaderTextureCDX : public IShaderTexture
{
  using TextureType = CDXTexture;
public:
  CShaderTextureCDX() = default;
  CShaderTextureCDX(TextureType* texture) : m_texture(texture) {}
  CShaderTextureCDX(TextureType& texture) : m_texture(&texture) {}

  // Destructor
  // Don't delete texture since it wasn't created here
  ~CShaderTextureCDX() override = default;

  float GetWidth() const override { return static_cast<float>(m_texture->GetWidth()); }
  float GetHeight() const override { return static_cast<float>(m_texture->GetHeight()); }

  void SetTexture(TextureType* newTexture) { m_texture = newTexture; }

  ID3D11ShaderResourceView *GetShaderResource() const { return m_texture->GetShaderResource(); }

  TextureType* GetPointer() { return m_texture; }

private:
  TextureType* m_texture = nullptr;
};

class ShaderLutDX: public IShaderLut
{
public:
  ShaderLutDX() = default;
  ShaderLutDX(std::string id, std::string path, IShaderSampler* sampler, IShaderTexture* texture)
    : IShaderLut(id, path)
  {
    m_sampler.reset(sampler);
    m_texture.reset(texture);
  }

  // Destructor
  ~ShaderLutDX() override = default;

  IShaderSampler* GetSampler() override { return m_sampler.get(); }
  IShaderTexture* GetTexture() override { return m_texture.get(); }

private:
  std::unique_ptr<IShaderSampler> m_sampler;
  std::unique_ptr<IShaderTexture> m_texture;
};

IShaderSampler* CreateLUTSampler(RETRO::CRenderContext &context, const VideoShaderLut &lut); //! @todo Move context to class
IShaderTexture* CreateLUTexture(const VideoShaderLut &lut);
D3D11_TEXTURE_ADDRESS_MODE TranslateWrapType(WRAP_TYPE wrap);

using ShaderLutsDX = std::vector<std::shared_ptr<ShaderLutDX>>;
}
}
