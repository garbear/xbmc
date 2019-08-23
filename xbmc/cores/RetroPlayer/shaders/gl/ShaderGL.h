/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTextureGL.h"
#include "cores/RetroPlayer/shaders/IShader.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTypesGL.h"
#include "rendering/gl/GLShader.h"
#include "guilib/TextureGL.h"

#include <stdint.h>

namespace KODI
{
namespace RETRO
{

class CRenderContext;

}

namespace SHADER
{
class CShaderGL : public IShader
{
public:
  CShaderGL(RETRO::CRenderContext &context);
//  ~CShaderGL() override;
  bool CreateVertexBuffer(unsigned vertCount, unsigned vertSize) override;

  // implementation of IShader
  bool Create(const std::string& shaderSource, const std::string& shaderPath, ShaderParameterMap shaderParameters,
         IShaderSampler* sampler, ShaderLutVec luts, float2 viewPortSize, unsigned frameCountMod = 0) override;
  void Render(IShaderTexture* source, IShaderTexture* target) override;
  void SetSizes(const float2& prevSize, const float2& nextSize) override;
  void PrepareParameters(CPoint dest[4], bool isLastPass, uint64_t frameCount) override;
  void UpdateMVP() override;
  bool CreateInputBuffer() override;
  void UpdateInputBuffer(uint64_t frameCount);
  void GetUniformLocs();

protected:
  void SetShaderParameters();

private:
  struct uniformInputs
  {
    float2 video_size;
    float2 texture_size;
    float2 output_size;
    GLint frame_count;
    GLfloat frame_direction;
  };

  // Currently loaded shader's source code
  std::string m_shaderSource;

  // Currently loaded shader's relative path
  std::string m_shaderPath;

  // Array of shader parameters
  ShaderParameterMap m_shaderParameters;

  // Look-up textures that the shader uses
  ShaderLutVec m_luts; // todo: back to DX maybe

  // Resolution of the input of the shader
  float2 m_inputSize;

  // Resolution of the output of the shader
  float2 m_outputSize;

  // Resolution of the viewport/window
  float2 m_viewportSize;

  // Resolution of the texture that holds the input
  //float2 m_textureSize;

  GLuint m_shaderProgram;

  // Projection matrix
  std::array<std::array<GLfloat, 4>, 4> m_MVP;

  float m_VertexCoords[4][3];
  float m_colors[4][3];
  float m_TexCoords[4][2];
  unsigned int m_indices[2][3];

  // Value to modulo (%) frame count with
  // Unused if 0
  unsigned m_frameCountMod = 0;

  GLint m_FrameDirectionLoc = -1;
  GLint m_FrameCountLoc = -1;
  GLint m_OutputSizeLoc = -1;
  GLint m_TextureSizeLoc = -1;
  GLint m_InputSizeLoc = -1;
  GLint m_MVPMatrixLoc = -1;

  GLuint VAO, EBO, VBO[3];

private:
  uniformInputs GetInputData(uint64_t frameCount = 0);

  // Construction parameters
  RETRO::CRenderContext &m_context;
};
}
}
