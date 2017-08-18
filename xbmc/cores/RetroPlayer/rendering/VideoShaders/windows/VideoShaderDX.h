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

#include "VideoShaderTextureDX.h"
#include "cores/RetroPlayer/rendering/VideoShaders/IVideoShader.h"
#include "cores/VideoPlayer/VideoRenderers/VideoShaders/WinVideoFilter.h"
#include "guilib/D3DResource.h"

#include <stdint.h>

namespace KODI
{
namespace RETRO
{
  class CRenderContext;
}

namespace SHADER
{

// TODO: make renderer independent
// libretro's "Common shaders"
// Spec here: https://github.com/libretro/common-shaders/blob/master/docs/README
class CVideoShaderDX : public CWinShader, public IVideoShader
{
public:
  CVideoShaderDX(RETRO::CRenderContext &context);
  ~CVideoShaderDX() override;

  // implementation of IVideoShader
  bool Create(const std::string& shaderSource, const std::string& shaderPath, ShaderParameters shaderParameters,
    IShaderSampler* sampler, IShaderLuts luts, float2 viewPortSize, unsigned frameCountMod = 0) override;
  void Render(IShaderTexture* source, IShaderTexture* target) override;
  void SetSizes(const float2& prevSize, const float2& nextSize) override;
  void PrepareParameters(CPoint dest[4], bool isLastPass, uint64_t frameCount) override;
  CD3DEffect& GetEffect();
  void UpdateMVP() override;
  bool CreateInputBuffer() override;
  void UpdateInputBuffer(uint64_t frameCount);

  // expose these from CWinShader
  bool CreateVertexBuffer(unsigned vertCount, unsigned vertSize) override;
  bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC *layout, unsigned numElements) override;

protected:
  void SetShaderParameters(CD3DTexture& sourceTexture);

private:
  struct cbInput
  {
    XMFLOAT2 video_size;
    XMFLOAT2 texture_size;
    XMFLOAT2 output_size;
    float frame_count;
    float frame_direction;
  };

  // Currently loaded shader's source code
  std::string m_shaderSource;

  // Currently loaded shader's relative path
  std::string m_shaderPath;

  // Array of shader parameters
  ShaderParameters m_shaderParameters;

  // Sampler state
  ID3D11SamplerState* m_pSampler = nullptr;

  // Look-up textures that the shader uses
  IShaderLuts m_luts; // todo: back to DX maybe

  // Resolution of the input of the shader
  float2 m_inputSize;

  // Resolution of the output of the shader
  float2 m_outputSize;

  // Resolution of the viewport/window
  float2 m_viewportSize;

  // Resolution of the texture that holds the input
  //float2 m_textureSize;

  // Holds the data bount to the input cbuffer (cbInput here)
  ID3D11Buffer* m_pInputBuffer = nullptr;

  // Projection matrix
  XMFLOAT4X4 m_MVP;

  // Value to modulo (%) frame count with
  // Unused if 0
  unsigned m_frameCountMod = 0;

private:
  cbInput GetInputData(uint64_t frameCount = 0);

  // Construction parameters
  RETRO::CRenderContext &m_context;
};

}
}
