/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderDX.h"
#include "ShaderTextureDX.h"
#include "cores/RetroPlayer/shaders/IShaderPreset.h"
#include "cores/RetroPlayer/shaders/ShaderTypes.h"
#include "games/GameServices.h"
#include "utils/Geometry.h"

#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>

namespace ADDON
{
  class CShaderPreset;
  class CShaderPresetAddon;
}

namespace KODI
{
namespace RETRO
{
  class CRenderContext;
}

namespace SHADER
{

class IShaderTexture;

class CShaderPresetDX : public IShaderPreset
{
public:
  // Instance of CShaderPreset
  explicit CShaderPresetDX(RETRO::CRenderContext &context, unsigned videoWidth = 0, unsigned videoHeight = 0);
  ~CShaderPresetDX() override;

  // implementation of IShaderPreset
  bool ReadPresetFile(const std::string& presetPath) override;
  bool RenderUpdate(const CPoint dest[], IShaderTexture* source, IShaderTexture* target) override;
  void SetSpeed(double speed) override { m_speed = speed; }
  void SetVideoSize(const unsigned videoWidth, const unsigned videoHeight) override;
  bool SetShaderPreset(const std::string& shaderPresetPath) override;
  const std::string& GetShaderPreset() const override;
  ShaderPassVec& GetPasses() override { return m_passes; }

  bool Update();
  //CShaderTextureDX* GetFirstTexture();

private:
  bool CreateShaderTextures();
  bool CreateShaders();
  bool CreateSamplers();
  bool CreateLayouts();
  bool CreateBuffers();
  void UpdateViewPort();
  void UpdateViewPort(CRect viewPort);
  void UpdateMVPs();
  void DisposeShaders();
  void PrepareParameters(const IShaderTexture* texture, const CPoint dest[]);
  void RenderShader(IShader* shader, IShaderTexture* source, IShaderTexture* target) const;
  bool HasPathFailed(const std::string& path) const;

  // Construction parameters
  RETRO::CRenderContext &m_context;

  // Relative path of the currently loaded shader preset
  // If empty, it means that a preset is not currently loaded
  std::string m_presetPath;

  // Video shaders for the shader passes
  std::vector<std::unique_ptr<CShaderDX>> m_pShaders;

  // Intermediate textures used for pixel shader passes
  std::vector<std::unique_ptr<CShaderTextureCD3D>> m_pShaderTextures;

  // First texture (this won't be needed when we have RGB rendering
  std::unique_ptr<CShaderTextureCD3D> firstTexture;

  // Was the shader preset changed during the last frame?
  bool m_bPresetNeedsUpdate = true;

  // Size of the viewport
  float2 m_outputSize;

  // The size of the input texture itself
  // Power-of-two sized.
  float2 m_textureSize;

  // Size of the actual source video data (ie. 160x144 for the Game Boy)
  float2 m_videoSize;

  // Number of frames that have passed
  float m_frameCount = 0.0f;

  // Point/nearest neighbor sampler
  ID3D11SamplerState* m_pSampNearest = nullptr;

  // Linear sampler
  ID3D11SamplerState* m_pSampLinear = nullptr;

  // Set of paths of presets that are known to not load correctly
  // Should not contain "" (empty path) because this signifies that a preset is not loaded
  std::set<std::string> m_failedPaths;

  // Array of vertices that comprise the full viewport
  CPoint m_dest[4];

  // Playback speed
  double m_speed = 0.0;

  ShaderParameterMap GetShaderParameters(const std::vector<ShaderParameter> &parameters, const std::string& sourceStr) const;

  ShaderPassVec m_passes;
};

}
}
