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
#include "guilib/TextureGL.h"
#include "rendering/gl/GLShader.h"

#include <array>
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
  CShaderGL(RETRO::CRenderContext& context);
  ~CShaderGL() override = default;

  // Implementation of IShader
  bool Create(const std::string& shaderSource,
              const std::string& shaderPath,
              ShaderParameterMap shaderParameters,
              ShaderLutVec luts,
              float2 viewPortSize,
              unsigned passIdx,
              unsigned frameCountMod = 0) override;
  void Render(IShaderTexture* source, IShaderTexture* target) override;
  void SetSizes(const float2& prevSize,
                const float2& prevTextureSize,
                const float2& nextSize) override;
  void PrepareParameters(CPoint dest[4],
                         IShaderTexture* sourceTexture,
                         const std::vector<std::unique_ptr<IShaderTexture>>& pShaderTextures,
                         const std::vector<std::unique_ptr<IShader>>& pShaders,
                         uint64_t frameCount) override;
  void UpdateMVP() override;

private:
  struct uniformInputs
  {
    float2 video_size;
    float2 texture_size;
    float2 output_size;
    GLint frame_count;
    GLfloat frame_direction;
  };

  struct uniformFrameInputs
  {
    float2 input_size;
    float2 texture_size;
    GLuint texture;
  };

  void UpdateUniformInputs(IShaderTexture* sourceTexture,
                           const std::vector<std::unique_ptr<IShaderTexture>>& pShaderTextures,
                           const std::vector<std::unique_ptr<IShader>>& pShaders,
                           uint64_t frameCount);
  uniformInputs GetInputData(uint64_t frameCount = 0);
  uniformFrameInputs GetFrameInputData(GLuint texture);
  uniformFrameInputs GetFrameUniformInputs() { return m_uniformFrameInputs; }
  void GetUniformLocs();
  void SetShaderParameters();

  // Currently loaded shader's source code
  std::string m_shaderSource;

  // Currently loaded shader's relative path
  std::string m_shaderPath;

  // Struct with all parameters pertaining to the shader
  ShaderParameterMap m_shaderParameters;

  // Look-up textures pertaining to the shader
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
  std::array<std::array<GLfloat, 4>, 4> m_MVP;

  // Index of the video shader pass
  unsigned m_passIdx;

  // Value to modulo (%) frame count with
  // Unused if 0
  unsigned m_frameCountMod = 0;

  GLuint m_shaderProgram = 0;
  GLubyte m_indices[4];
  float m_VertexCoords[4][3];
  float m_colors[4][3];
  float m_TexCoords[4][2];

  uniformInputs m_uniformInputs;
  uniformFrameInputs m_uniformFrameInputs;
  std::vector<uniformFrameInputs> m_passesUniformFrameInputs;

  GLint m_FrameDirectionLoc = -1;
  GLint m_FrameCountLoc = -1;
  GLint m_OutputSizeLoc = -1;
  GLint m_TextureSizeLoc = -1;
  GLint m_InputSizeLoc = -1;
  GLint m_MVPMatrixLoc = -1;

#ifndef HAS_GLES
  GLuint VAO = 0;
#endif
  GLuint VBO[3] = {};
  GLuint EBO = 0;
};
} // namespace SHADER
} // namespace KODI
