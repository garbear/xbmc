/*
 *  Copyright (C) 2017-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTextureDX.h"
#include "cores/RetroPlayer/shaders/IShader.h"
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

//! @todo Make renderer independent
// Libretro's "Common shaders"
// Spec here: https://github.com/libretro/common-shaders/blob/master/docs/README
class CShaderDX : public CWinShader, public IShader
{
public:
  CShaderDX(RETRO::CRenderContext& context);
  ~CShaderDX() override;

  // Implementation of IShader
  bool Create(const std::string& shaderSource,
              const std::string& shaderPath,
              ShaderParameterMap shaderParameters,
              IShaderSampler* sampler,
              ShaderLutVec luts,
              float2 viewPortSize,
              unsigned frameCountMod = 0) override;
  void Render(IShaderTexture* source, IShaderTexture* target) override;
  void SetSizes(const float2& prevSize,
                const float2& prevTextureSize,
                const float2& nextSize) override;
  void PrepareParameters(CPoint dest[4], bool isLastPass, uint64_t frameCount) override;
  void UpdateMVP() override;

  /*!
   * \brief Construct the vertex buffer that will be used to render the shader
   * \param vertCount Number of vertices to construct. Commonly 4, for rectangular screens.
   * \param vertSize Size of each vertex's data in bytes
   * \return False if creating the vertex buffer failed, true otherwise.
   */
  bool CreateVertexBuffer(unsigned vertCount, unsigned vertSize);

  /*!
   * \brief Creates the data layout of the input-assembler stage
   * \param layout Description of the inputs to the vertex shader
   * \param numElements Number of inputs to the vertex shader
   * \return False if creating the input layout failed, true otherwise.
   */
  bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* layout, unsigned numElements);

  /*!
   * \brief Creates the buffer that will be used to send "input" (as per the spec) data to the
   * shader \return False if creating the input buffer failed, true otherwise.
   */
  bool CreateInputBuffer();

private:
  struct cbInput
  {
    XMFLOAT2 video_size;
    XMFLOAT2 texture_size;
    XMFLOAT2 output_size;
    float frame_count;
    float frame_direction;
  };

  void UpdateInputBuffer(uint64_t frameCount);
  cbInput GetInputData(uint64_t frameCount = 0);
  void SetShaderParameters(CD3DTexture& sourceTexture);

  // Currently loaded shader's source code
  std::string m_shaderSource;

  // Currently loaded shader's relative path
  std::string m_shaderPath;

  // Array of shader parameters
  ShaderParameterMap m_shaderParameters;

  // Sampler state
  ID3D11SamplerState* m_pSampler = nullptr;

  // Look-up textures that the shader uses
  ShaderLutVec m_luts; //! @todo Back to DX maybe

  // Resolution of the input of the shader
  float2 m_inputSize;

  // Resolution of the texture that holds the input
  float2 m_inputTextureSize;

  // Resolution of the output viewport of the shader
  float2 m_outputSize;

  // Resolution of the destination rectangle of the shader
  float2 m_destSize;

  // Resolution of the viewport/window
  float2 m_viewportSize;

  // Projection matrix
  XMFLOAT4X4 m_MVP;

  // Value to modulo (%) frame count with
  // Unused if 0
  unsigned m_frameCountMod = 0;

  // Holds the data bound to the input cbuffer (cbInput here)
  ID3D11Buffer* m_pInputBuffer = nullptr;

  // Construction parameters
  RETRO::CRenderContext& m_context;
};

} // namespace SHADER
} // namespace KODI
