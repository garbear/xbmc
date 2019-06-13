/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderGL.h"
#include "ShaderTextureGL.h"
#include "Application.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/IShaderLut.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "ShaderUtilsGL.h"

using namespace KODI;
using namespace SHADER;

CShaderGL::CShaderGL(RETRO::CRenderContext &context) :
        m_context(context)
{
}

bool CShaderGL::Create(const std::string& shaderSource, const std::string& shaderPath, ShaderParameterMap shaderParameters,
        IShaderSampler* sampler, ShaderLutVec luts,
        float2 viewPortSize, unsigned frameCountMod)
{
  //TODO:Remove sampler input from IShader.h
  if(shaderPath.empty())
  {
    CLog::Log(LOGERROR, "ShaderDX: Can't load empty shader path");
    return false;
  }

  m_shaderSource = shaderSource;
  m_shaderPath = shaderPath;
  m_shaderParameters = shaderParameters;
  m_luts = luts;
  m_viewportSize = viewPortSize;
  m_frameCountMod = frameCountMod;

  std::string shaderFile = CShaderUtilsGL::FileToString(shaderSource);

  std::string vertexShaderSourceStr = "#define VERTEX\n" + shaderFile;
  std::string fragmentShaderSourceStr = "#define FRAGMENT\n" + shaderFile;
  const char *vertexShaderSource = vertexShaderSourceStr.c_str();
  const char *fragmentShaderSource = fragmentShaderSourceStr.c_str();

  GLuint vShader;
  vShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vShader, 1, &vertexShaderSource, NULL);

  GLuint fShader;
  fShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fShader, 1, &fragmentShaderSource, NULL);

  m_shaderProgram = glCreateProgram();
  glAttachShader(m_shaderProgram, vShader);
  glAttachShader(m_shaderProgram, fShader);
  glLinkProgram(m_shaderProgram);
  glDeleteShader(vShader);
  glDeleteShader(fShader);

  return true;
}

void CShaderGL::Render(IShaderTexture *source, IShaderTexture *target)
{
  CShaderTextureGL* sourceGL = static_cast<CShaderTextureGL*> (source);
  GLuint texture = sourceGL->GetPointer()->getMTexture();
  glUseProgram(m_shaderProgram);
  glBindTexture(GL_TEXTURE_2D, texture);
  SetShaderParameters();
  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void CShaderGL::SetShaderParameters()
{
  glUniformMatrix4fv(m_MVPMatrixLoc, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(&m_MVP));


  glGenVertexArrays(1, &VAO);
  glGenBuffers(4, VBO);
  glBindVertexArray(VAO);

  GLuint VertexCoordIndex = glGetAttribLocation(m_shaderProgram, "VertexCoord");
  glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(m_VertexCoords), m_VertexCoords, GL_STATIC_DRAW);
  glVertexAttribPointer(VertexCoordIndex, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(VertexCoordIndex);

  GLuint ColorIndex = glGetAttribLocation(m_shaderProgram, "COLOR");
  glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(m_colors), m_colors, GL_STATIC_DRAW);
  glVertexAttribPointer(ColorIndex, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(ColorIndex);

  GLuint TexCoordIndex = glGetAttribLocation(m_shaderProgram, "TexCoord");
  glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(m_TexCoords), m_TexCoords, GL_STATIC_DRAW);
  glVertexAttribPointer(TexCoordIndex, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(TexCoordIndex);

  glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(m_indices), m_indices, GL_STATIC_DRAW);
}

void CShaderGL::PrepareParameters(CPoint *dest, bool isLastPass, uint64_t frameCount)
{
  UpdateInputBuffer(frameCount);

  if (!isLastPass)
  {
    // top left x,y
    m_VertexCoords[0][0] = -m_outputSize.x / 2;
    m_VertexCoords[0][1] = -m_outputSize.y / 2;
    // top right x,y
    m_VertexCoords[1][0] = m_outputSize.x / 2;
    m_VertexCoords[1][1] = -m_outputSize.y / 2;
    // bottom right x,y
    m_VertexCoords[2][0] = m_outputSize.x / 2;
    m_VertexCoords[2][1] = m_outputSize.y / 2;
    // bottom left x,y
    m_VertexCoords[3][0] = -m_outputSize.x / 2;
    m_VertexCoords[3][1] = m_outputSize.y / 2;
  }
  else  // last pass
  {
    // top left x,y
    m_VertexCoords[0][0] = dest[0].x - m_outputSize.x / 2;
    m_VertexCoords[0][1] = dest[0].y - m_outputSize.y / 2;
    // top right x,y
    m_VertexCoords[1][0] = dest[1].x - m_outputSize.x / 2;
    m_VertexCoords[1][1] = dest[1].y - m_outputSize.y / 2;
    // bottom right x,y
    m_VertexCoords[2][0] = dest[2].x - m_outputSize.x / 2;
    m_VertexCoords[2][1] = dest[2].y - m_outputSize.y / 2;
    // bottom left x,y
    m_VertexCoords[3][0] = dest[3].x - m_outputSize.x / 2;
    m_VertexCoords[3][1] = dest[3].y - m_outputSize.y / 2;
  }

  // top left z, tu, tv, r, g, b
  m_VertexCoords[0][2] = 0;
  m_TexCoords[0][0] = 0;
  m_TexCoords[0][1] = 0;
  m_colors[0][0] = 0;
  m_colors[0][1] = 0;
  m_colors[0][2] = 0;

  // top right z, tu, tv, r, g, b
  m_VertexCoords[1][2] = 0;
  m_TexCoords[1][0] = 1;
  m_TexCoords[1][1] = 0;
  m_colors[1][0] = 0;
  m_colors[1][1] = 0;
  m_colors[1][2] = 0;

  // bottom right z, tu, tv, r, g, b
  m_VertexCoords[2][2] = 0;
  m_TexCoords[2][0] = 1;
  m_TexCoords[2][1] = 1;
  m_colors[2][0] = 0;
  m_colors[2][1] = 0;
  m_colors[2][2] = 0;

  // bottom left z, tu, tv, r, g, b
  m_VertexCoords[3][2] = 0;
  m_TexCoords[3][0] = 0;
  m_TexCoords[3][1] = 1;
  m_colors[3][0] = 0;
  m_colors[3][1] = 0;
  m_colors[3][2] = 0;

  m_indices[0][0] = 0;
  m_indices[0][1] = 1;
  m_indices[0][2] = 3;
  m_indices[1][0] = 1;
  m_indices[1][1] = 2;
  m_indices[1][2] = 3;
}

void CShaderGL::UpdateMVP()
{
  GLfloat xScale = 1.0f / m_outputSize.x * 2.0f;
  GLfloat yScale = -1.0f / m_outputSize.y * 2.0f;

  // Update projection matrix
  m_MVP = {{
    {xScale, 0, 0, 0},
    {0, yScale, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
  }};
}

bool CShaderGL::GetUniformLocs()
{
  CShaderGL::m_FrameDirectionLoc = glGetUniformLocation(m_shaderProgram, "FrameDirection");
  CShaderGL::m_FrameCountLoc = glGetUniformLocation(m_shaderProgram, "FrameCount");
  CShaderGL::m_OutputSizeLoc = glGetUniformLocation(m_shaderProgram, "OutputSize");
  CShaderGL::m_TextureSizeLoc = glGetUniformLocation(m_shaderProgram, "TextureSize");
  CShaderGL::m_InputSizeLoc = glGetUniformLocation(m_shaderProgram, "InputSize");
  CShaderGL::m_MVPMatrixLoc = glGetUniformLocation(m_shaderProgram, "MVPMatrix");

  return !(m_FrameDirectionLoc == -1 ||
    m_FrameCountLoc == -1 ||
    m_OutputSizeLoc == -1 ||
    m_TextureSizeLoc == -1 ||
    m_InputSizeLoc == -1 ||
    m_MVPMatrixLoc == -1);
}

//TODO:Change name of this method in IShader.h to CreateInputs
bool CShaderGL::CreateInputBuffer()
{
  if(!GetUniformLocs())
  {
    //TODO:Meaningful error log
    return false;
  }
  UpdateInputBuffer(0);
  return true;
}

//TODO:Change name of this method in IShader.h to UpdateInputs
void CShaderGL::UpdateInputBuffer(uint64_t frameCount)
{
  uniformInputs inputInitData = GetInputData(frameCount);
  glUniform1i(m_FrameDirectionLoc, inputInitData.frame_direction);
  glUniform1i(m_FrameCountLoc, inputInitData.frame_count);
  glUniform2f(m_OutputSizeLoc, inputInitData.output_size.x, inputInitData.output_size.y);
  glUniform2f(m_TextureSizeLoc, inputInitData.texture_size.x, inputInitData.texture_size.y);
  glUniform2f(m_InputSizeLoc, inputInitData.video_size.x, inputInitData.video_size.y);
}

CShaderGL::uniformInputs CShaderGL::GetInputData(uint64_t frameCount)
{
  if (m_frameCountMod != 0)
    frameCount %= m_frameCountMod;

  uniformInputs input = {
    // Resolution of texture passed to the shader
    { m_inputSize },       // video_size
    { m_inputSize },      // texture_size
    // As per the spec, this is the viewport resolution (not the
    // output res of each shader)
    { m_viewportSize },   // output_size
    // Current frame count that can be modulo'ed
    static_cast<GLint>(frameCount),  // frame_count
    // Time always flows forward
    1                 // frame_direction
  };

  return input;
}

void CShaderGL::SetSizes(const float2 &prevSize, const float2 &nextSize)
{
  m_inputSize = prevSize;
  m_outputSize = nextSize;
}
