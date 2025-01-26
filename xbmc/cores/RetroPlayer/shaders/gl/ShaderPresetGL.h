/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderGL.h"
#ifndef HAS_GLES
#include "ShaderTextureGL.h"
#else
#include "ShaderTextureGLES.h"
#endif
#include "cores/RetroPlayer/shaders/IShaderPreset.h"
#include "cores/RetroPlayer/shaders/ShaderTypes.h"
#include "games/GameServices.h"
#include "utils/Geometry.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "system_gl.h"

namespace ADDON
{
class CShaderPreset;
class CShaderPresetAddon;
} // namespace ADDON

namespace KODI
{
namespace RETRO
{
class CRenderContext;
}

namespace SHADER
{

class IShaderTexture;
class CShaderPresetGL : public IShaderPreset
{
public:
  // Instance of CShaderPreset
  explicit CShaderPresetGL(RETRO::CRenderContext& context,
                           unsigned videoWidth = 0,
                           unsigned videoHeight = 0);
  ~CShaderPresetGL() override;

  // Implementation of IShaderPreset
  bool ReadPresetFile(const std::string& presetPath) override;
  bool RenderUpdate(const CPoint dest[], IShaderTexture* source, IShaderTexture* target) override;
  void SetSpeed(double speed) override { m_speed = speed; }
  void SetVideoSize(const unsigned videoWidth, const unsigned videoHeight) override;
  bool SetShaderPreset(const std::string& shaderPresetPath) override;
  const std::string& GetShaderPreset() const override;
  ShaderPassVec& GetPasses() override { return m_passes; }
  bool Update();

private:
  void UpdateViewPort();
  void UpdateViewPort(CRect viewPort);
  void UpdateMVPs();
  void PrepareParameters(const CPoint dest[],
                         IShaderTexture* source,
                         IShaderTexture* target);
  bool CreateShaders();
  bool CreateShaderTextures();
  void RenderShader(IShader* shader, IShaderTexture* source, IShaderTexture* target) const;
  void DisposeShaders();
  bool HasPathFailed(const std::string& path) const;
  ShaderParameterMap GetShaderParameters(const std::vector<ShaderParameter>& parameters,
                                         const std::string& sourceStr) const;

  // Construction parameters
  RETRO::CRenderContext& m_context;

  // Relative path of the currently loaded shader preset
  // If empty, it means that a preset is not currently loaded
  std::string m_presetPath;

  // Set of paths of presets that are known to not load correctly
  // Should not contain "" (empty path) because this signifies that a preset is not loaded
  std::set<std::string> m_failedPaths;

  // All video shader passes of the currently loaded preset
  ShaderPassVec m_passes;

  // Video shaders for the shader passes
  std::vector<std::unique_ptr<IShader>> m_pShaders;

  // Intermediate textures used for pixel shader passes
  std::vector<std::unique_ptr<IShaderTexture>> m_pShaderTextures;

  // Was the shader preset changed during the last frame?
  bool m_bPresetNeedsUpdate = true;

  // Size of the viewport
  float2 m_outputSize;

  // Size of the actual source video data (ie. 160x144 for the Game Boy)
  float2 m_videoSize;

  // Array of vertices that comprise the full viewport
  CPoint m_dest[4];

  // Number of frames that have passed
  float m_frameCount = 0.0f;

  // Playback speed
  double m_speed = 1.0;
};

} // namespace SHADER
} // namespace KODI
