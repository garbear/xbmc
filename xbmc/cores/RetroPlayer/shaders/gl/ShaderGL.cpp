/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderGL.h"

#include "ShaderTextureGL.h"
#include "ShaderUtilsGL.h"
#include "application/Application.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/IShaderLut.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace SHADER;

CShaderGL::CShaderGL(RETRO::CRenderContext& context)
{
}

CShaderGL::~CShaderGL()
{
  glDeleteBuffers(1, &EBO);
  glDeleteBuffers(3, VBO);
  glDeleteVertexArrays(1, &VAO);
}

bool CShaderGL::Create(const std::string& shaderSource,
                       const std::string& shaderPath,
                       ShaderParameterMap shaderParameters,
                       ShaderLutVec luts,
                       float2 viewPortSize,
                       unsigned passIdx,
                       unsigned frameCountMod)
{
  if (shaderPath.empty())
  {
    CLog::Log(LOGERROR, "ShaderGL: Can't load empty shader path");
    return false;
  }

  m_shaderSource = CShaderUtilsGL::StripParameterPragmas(shaderSource);
  m_shaderPath = shaderPath;
  m_shaderParameters = shaderParameters;
  m_luts = luts;
  m_viewportSize = viewPortSize;
  m_passIdx = passIdx;
  m_frameCountMod = frameCountMod;

  std::string defineVersion = CShaderUtilsGL::GetGLSLVersion(m_shaderSource);
  std::string defineVertex = "#define VERTEX\n#define PARAMETER_UNIFORM\n";
  std::string defineFragment = "#define FRAGMENT\n#define PARAMETER_UNIFORM\n";

  std::string vertexShaderSourceStr = defineVersion + defineVertex + m_shaderSource;
  std::string fragmentShaderSourceStr = defineVersion + defineFragment + m_shaderSource;
  const char* vertexShaderSource = vertexShaderSourceStr.c_str();
  const char* fragmentShaderSource = fragmentShaderSourceStr.c_str();

  GLuint vShader;
  vShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vShader);

  GLuint fShader;
  fShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fShader); //! @todo Make this good

  m_shaderProgram = glCreateProgram();
  glAttachShader(m_shaderProgram, vShader);
  glAttachShader(m_shaderProgram, fShader);
  glBindAttribLocation(m_shaderProgram, 0, "VertexCoord");
  glBindAttribLocation(m_shaderProgram, 1, "COLOR");
  glBindAttribLocation(m_shaderProgram, 2, "TexCoord");

  glLinkProgram(m_shaderProgram);
  glDeleteShader(vShader);
  glDeleteShader(fShader);

  glUseProgram(m_shaderProgram);
  GLint paramLoc = glGetUniformLocation(m_shaderProgram, "Texture");
  glUniform1i(paramLoc, 0);
  glUseProgram(0);

  GetUniformLocs();

  glGenVertexArrays(1, &VAO);
  glGenBuffers(3, VBO);
  glGenBuffers(1, &EBO);
  return true;
}

void CShaderGL::Render(IShaderTexture* source, IShaderTexture* target)
{
  auto* sourceGL = static_cast<CShaderTextureGL*>(source);
  sourceGL->GetPointer()->BindToUnit(0);

  if (sourceGL->IsMipmapped())
    glGenerateMipmap(GL_TEXTURE_2D);

  glUseProgram(m_shaderProgram);

  glBindVertexArray(VAO);

  SetShaderParameters();

  glUniformMatrix4fv(m_MVPMatrixLoc, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&m_MVP));

  glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(m_VertexCoords), m_VertexCoords, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(m_colors), m_colors, GL_STATIC_DRAW);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(m_TexCoords), m_TexCoords, GL_STATIC_DRAW);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(2);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_indices), m_indices, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glUseProgram(0);
}

void CShaderGL::SetSizes(const float2& prevSize,
                         const float2& prevTextureSize,
                         const float2& nextSize)
{
  m_inputSize = prevSize;
  m_inputTextureSize = prevTextureSize;
  m_outputSize = nextSize;
}

void CShaderGL::PrepareParameters(
    CPoint dest[4],
    IShaderTexture* sourceTexture,
    const std::vector<std::unique_ptr<IShaderTexture>>& pShaderTextures,
    const std::vector<std::unique_ptr<IShader>>& pShaders,
    uint64_t frameCount)
{
  if (m_passIdx + 1 != pShaders.size()) // Not last pass
  {
    // bottom left x,y
    m_VertexCoords[0][0] = -m_outputSize.x / 2;
    m_VertexCoords[0][1] = -m_outputSize.y / 2;
    // bottom right x,y
    m_VertexCoords[1][0] = m_outputSize.x / 2;
    m_VertexCoords[1][1] = -m_outputSize.y / 2;
    // top right x,y
    m_VertexCoords[2][0] = m_outputSize.x / 2;
    m_VertexCoords[2][1] = m_outputSize.y / 2;
    // top left x,y
    m_VertexCoords[3][0] = -m_outputSize.x / 2;
    m_VertexCoords[3][1] = m_outputSize.y / 2;

    // Set destination rectangle size
    m_destSize = m_outputSize;
  }
  else // Last pass
  {
    // bottom left x,y
    m_VertexCoords[0][0] = dest[3].x - m_outputSize.x / 2;
    m_VertexCoords[0][1] = dest[3].y - m_outputSize.y / 2;
    // bottom right x,y
    m_VertexCoords[1][0] = dest[2].x - m_outputSize.x / 2;
    m_VertexCoords[1][1] = dest[2].y - m_outputSize.y / 2;
    // top right x,y
    m_VertexCoords[2][0] = dest[1].x - m_outputSize.x / 2;
    m_VertexCoords[2][1] = dest[1].y - m_outputSize.y / 2;
    // top left x,y
    m_VertexCoords[3][0] = dest[0].x - m_outputSize.x / 2;
    m_VertexCoords[3][1] = dest[0].y - m_outputSize.y / 2;

    // Set destination rectangle size for the last pass
    m_destSize = {dest[2].x - dest[0].x, dest[2].y - dest[0].y};
  }

  // bottom left z, tu, tv, r, g, b
  m_VertexCoords[0][2] = 0;
  m_colors[0][0] = 0.0f;
  m_colors[0][1] = 0.0f;
  m_colors[0][2] = 0.0f;
  m_TexCoords[0][0] = 0.0f;
  m_TexCoords[0][1] = 1.0f;
  // bottom right z, tu, tv, r, g, b
  m_VertexCoords[1][2] = 0;
  m_colors[1][0] = 0.0f;
  m_colors[1][1] = 0.0f;
  m_colors[1][2] = 0.0f;
  m_TexCoords[1][0] = 1.0f;
  m_TexCoords[1][1] = 1.0f;
  // top right z, tu, tv, r, g, b
  m_VertexCoords[2][2] = 0;
  m_colors[2][0] = 0.0f;
  m_colors[2][1] = 0.0f;
  m_colors[2][2] = 0.0f;
  m_TexCoords[2][0] = 1.0f;
  m_TexCoords[2][1] = 0.0f;
  // top left z, tu, tv, r, g, b
  m_VertexCoords[3][2] = 0;
  m_colors[3][0] = 0.0f;
  m_colors[3][1] = 0.0f;
  m_colors[3][2] = 0.0f;
  m_TexCoords[3][0] = 0.0f;
  m_TexCoords[3][1] = 0.0f;

  // Determines order of triangle strip
  m_indices[0] = 0;
  m_indices[1] = 1;
  m_indices[2] = 3;
  m_indices[3] = 2;

  UpdateUniformInputs(sourceTexture, pShaderTextures, pShaders, frameCount);
}

void CShaderGL::UpdateMVP()
{
  GLfloat xScale = 1.0f / m_outputSize.x * 2.0f;
  GLfloat yScale = -1.0f / m_outputSize.y * 2.0f;

  // Update projection matrix
  m_MVP = {{{xScale, 0, 0, 0}, {0, yScale, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
}

void CShaderGL::UpdateUniformInputs(
    IShaderTexture* sourceTexture,
    const std::vector<std::unique_ptr<IShaderTexture>>& pShaderTextures,
    const std::vector<std::unique_ptr<IShader>>& pShaders,
    uint64_t frameCount)
{
  m_uniformInputs = GetInputData(frameCount);

  if (m_passIdx) // Not first pass
  {
    auto* shaderTextureGL = static_cast<CShaderTextureGL*>(pShaderTextures[m_passIdx - 1].get());
    m_uniformFrameInputs = GetFrameInputData(shaderTextureGL->GetPointer()->getMTexture());
  }
  else // First pass
  {
    auto* sourceTextureGL = static_cast<CShaderTextureGL*>(sourceTexture);
    m_uniformFrameInputs = GetFrameInputData(sourceTextureGL->GetPointer()->getMTexture());
  }

  // Set frame uniforms of previous passes
  m_passesUniformFrameInputs.clear();

  for (unsigned i = 0; i < m_passIdx + 1; ++i)
  {
    auto* shader = static_cast<CShaderGL*>(pShaders[i].get());
    UniformFrameInputs frameInput = shader->GetFrameUniformInputs();
    m_passesUniformFrameInputs.emplace_back(frameInput);
  }
}

CShaderGL::UniformInputs CShaderGL::GetInputData(uint64_t frameCount)
{
  if (m_frameCountMod != 0)
    frameCount %= m_frameCountMod;

  UniformInputs input = {
      {m_inputSize}, // video_size
      {m_inputTextureSize}, // texture_size
      {m_destSize}, // output_size
      // Current frame count that can be modulo'ed
      static_cast<GLint>(frameCount), // frame_count
      // Time always flows forward
      1.0f // frame_direction
  };
  return input;
}

CShaderGL::UniformFrameInputs CShaderGL::GetFrameInputData(GLuint texture)
{
  UniformFrameInputs frameInput = {
      {m_inputSize}, // input_size
      {m_inputTextureSize}, // texture_size
      texture // texture
  };
  return frameInput;
}

void CShaderGL::GetUniformLocs()
{
  m_FrameDirectionLoc = glGetUniformLocation(m_shaderProgram, "FrameDirection");
  m_FrameCountLoc = glGetUniformLocation(m_shaderProgram, "FrameCount");
  m_OutputSizeLoc = glGetUniformLocation(m_shaderProgram, "OutputSize");
  m_TextureSizeLoc = glGetUniformLocation(m_shaderProgram, "TextureSize");
  m_InputSizeLoc = glGetUniformLocation(m_shaderProgram, "InputSize");
  m_MVPMatrixLoc = glGetUniformLocation(m_shaderProgram, "MVPMatrix");
}

void CShaderGL::SetShaderParameters()
{
  unsigned int textureUnit = 1; // GL_TEXTURE0 is used by source texture

  // Set shader uniforms
  glUniform1f(m_FrameDirectionLoc, m_uniformInputs.frame_direction);
  glUniform1i(m_FrameCountLoc, m_uniformInputs.frame_count);
  glUniform2f(m_OutputSizeLoc, m_uniformInputs.output_size.x, m_uniformInputs.output_size.y);
  glUniform2f(m_TextureSizeLoc, m_uniformInputs.texture_size.x, m_uniformInputs.texture_size.y);
  glUniform2f(m_InputSizeLoc, m_uniformInputs.video_size.x, m_uniformInputs.video_size.y);

  // Set lookup textures
  for (const auto& lut : m_luts)
  {
    auto* texture = static_cast<CShaderTextureGL*>(lut->GetTexture());
    if (texture != nullptr)
    {
      GLint paramLoc = glGetUniformLocation(m_shaderProgram, lut->GetID().c_str());
      glUniform1i(paramLoc, textureUnit);
      texture->GetPointer()->BindToUnit(textureUnit);
      textureUnit++;
    }
  }

  // Set FBO textures
  for (unsigned i = 0; i < m_passIdx + 1; ++i)
  {
    GLint paramLoc;
    std::string paramPass = i ? "Pass" + std::to_string(i) : "Orig";
    std::string paramPassPrev = "PassPrev" + std::to_string(m_passIdx + 1 - i);

    paramLoc = glGetUniformLocation(m_shaderProgram, (paramPass + "Texture").c_str());
    glUniform1i(paramLoc, textureUnit);
    paramLoc = glGetUniformLocation(m_shaderProgram, (paramPassPrev + "Texture").c_str());
    glUniform1i(paramLoc, textureUnit);
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_passesUniformFrameInputs[i].texture);
    textureUnit++;

    paramLoc = glGetUniformLocation(m_shaderProgram, (paramPass + "TextureSize").c_str());
    glUniform2f(paramLoc, m_passesUniformFrameInputs[i].texture_size.x,
                m_passesUniformFrameInputs[i].texture_size.y);
    paramLoc = glGetUniformLocation(m_shaderProgram, (paramPassPrev + "TextureSize").c_str());
    glUniform2f(paramLoc, m_passesUniformFrameInputs[i].texture_size.x,
                m_passesUniformFrameInputs[i].texture_size.y);

    paramLoc = glGetUniformLocation(m_shaderProgram, (paramPass + "InputSize").c_str());
    glUniform2f(paramLoc, m_passesUniformFrameInputs[i].input_size.x,
                m_passesUniformFrameInputs[i].input_size.y);
    paramLoc = glGetUniformLocation(m_shaderProgram, (paramPassPrev + "InputSize").c_str());
    glUniform2f(paramLoc, m_passesUniformFrameInputs[i].input_size.x,
                m_passesUniformFrameInputs[i].input_size.y);
  }

  // Set #pragma parameters
  for (const auto& param : m_shaderParameters)
  {
    GLint paramLoc = glGetUniformLocation(m_shaderProgram, param.first.c_str());
    glUniform1f(paramLoc, param.second);
  }
}
