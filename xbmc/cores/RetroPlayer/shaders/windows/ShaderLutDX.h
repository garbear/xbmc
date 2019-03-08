/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderLut.h"
#include "cores/RetroPlayer/shaders/ShaderTypes.h"

#include <d3d11.h>
#include <memory>
#include <string>

namespace KODI
{
namespace RETRO
{
  class CRenderContext;
}

namespace SHADER
{

class IShaderSampler;
class IShaderTexture;
struct ShaderLut;

class CShaderLutDX: public IShaderLut
{
public:
  CShaderLutDX() = default;
  CShaderLutDX(const std::string& id, const std::string& path);

  // Destructor
  ~CShaderLutDX() override;

  // Implementation of IShaderLut
  bool Create(RETRO::CRenderContext &context, const ShaderLut &lut) override;
  IShaderSampler* GetSampler() override { return m_sampler.get(); }
  IShaderTexture* GetTexture() override { return m_texture.get(); }

private:
  static std::unique_ptr<IShaderSampler> CreateLUTSampler(RETRO::CRenderContext &context, const ShaderLut &lut); //! @todo Move context to class
  static std::unique_ptr<IShaderTexture> CreateLUTexture(const ShaderLut &lut);

  std::unique_ptr<IShaderSampler> m_sampler;
  std::unique_ptr<IShaderTexture> m_texture;
};

}
}
