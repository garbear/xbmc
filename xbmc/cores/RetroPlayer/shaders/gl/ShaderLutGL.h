
/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderLut.h"
#include "cores/RetroPlayer/shaders/ShaderTypes.h"

#include <GL/gl.h>
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

class IShaderTexture;
struct ShaderLut;

class CShaderLutGL : public IShaderLut
{
public:
  CShaderLutGL() = default;
  CShaderLutGL(const std::string& id, const std::string& path);

  //Destructor
  ~CShaderLutGL() override;

  //Implementation of IShaderLut
  bool Create(RETRO::CRenderContext &context, const ShaderLut &lut) override;
  IShaderSampler* GetSampler() override { return nullptr; }
  IShaderTexture* GetTexture() override { return m_texture.get(); }

private:
  static std::unique_ptr<IShaderSampler> CreateLUTSampler(RETRO::CRenderContext &context, const ShaderLut &lut);
  static std::unique_ptr<IShaderTexture> CreateLUTTexture(RETRO::CRenderContext &context, const ShaderLut &lut);

  std::unique_ptr<IShaderTexture> m_texture;
};

}
}