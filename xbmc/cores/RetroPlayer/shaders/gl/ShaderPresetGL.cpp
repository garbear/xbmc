/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetGL.h"
#include "cores/RetroPlayer/shaders/ShaderUtils.h"
#include "cores/RetroPlayer/shaders/gl/ShaderLutGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "cores/RetroPlayer/shaders/gl/ShaderGL.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"
#include "ServiceBroker.h"

#include <regex>

#define MAX_FLOAT 3.402823466E+38

using namespace KODI;
using namespace SHADER;

CShaderPresetGL::CShaderPresetGL(RETRO::CRenderContext &context, unsigned int videoWidth, unsigned int videoHeight)
  : m_context(context)
  , m_videoSize(videoWidth, videoHeight)
{
  m_textureSize = CShaderUtils::GetOptimalTextureSize(m_videoSize);

  CRect viewPort;
  m_context.GetViewPort(viewPort);
  m_outputSize = {viewPort.Width(), viewPort.Height()};
}

CShaderPresetGL::~CShaderPresetGL()
{
  DisposeShaders();
  // The gui is going to render after this, so apply the state required
  m_context.ApplyStateBlock();
}

ShaderParameterMap CShaderPresetGL::GetShaderParameters(const std::vector<ShaderParameter> &parameters, const std::string &sourceStr) const
{
  static const std::regex pragmaParamRegex("#pragma parameter ([a-zA-Z_][a-zA-Z0-9_]*)");
  std::smatch matches;

  std::vector<std::string> validParams;
  std::string::const_iterator searchStart(sourceStr.cbegin());
  while (regex_search(searchStart, sourceStr.cend(), matches, pragmaParamRegex))
  {
    validParams.push_back(matches[1].str());
    searchStart += matches.position() + matches.length();
  }

  ShaderParameterMap matchParams;
  for (const auto& match : validParams)   // for each param found in the source code
  {
    for (const auto& parameter : parameters)   // for each param found in the preset file
    {
      if (match == parameter.strId)  // if they match
      {
        // The add-on has already handled parsing and overwriting default
        // parameter values from the preset file. The final value we
        // should use is in the 'current' field.
        matchParams[match] = parameter.current;
        break;
      }
    }
  }

  return matchParams;
}

bool CShaderPresetGL::RenderUpdate(const CPoint *dest, IShaderTexture *source, IShaderTexture *target)
{
  // Save the viewport
  CRect viewPort;
  m_context.GetViewPort(viewPort);

  // Handle resizing of the viewport (window)
  UpdateViewPort(viewPort);

  // Update shaders/shader textures if required
  if (!Update())
    return false;

  PrepareParameters(target, dest);

  IShader* firstShader = m_pShaders.front().get();
  CShaderTextureGL* firstShaderTexture = m_pShaderTextures.front().get();
  IShader* lastShader = m_pShaders.back().get();
  int screenWidth = m_context.GetScreenWidth();
  int screenHeight = m_context.GetScreenHeight();

  const unsigned passesNum = static_cast<unsigned int>(m_pShaderTextures.size());

  if (passesNum == 1)
    firstShader->Render(source, target);
  else if (passesNum == 2)
  {
    // Initialize FBO
    firstShaderTexture->CreateFBO(screenWidth, screenHeight);
    // Apply first pass
    firstShaderTexture->BindFBO();
    RenderShader(firstShader, source, target);
    firstShaderTexture->UnbindFBO();
    // Apply last pass
    RenderShader(lastShader, firstShaderTexture, target);
  }
  else
  {
    // Initialize FBO
    firstShaderTexture->CreateFBO(screenWidth, screenHeight);
    // Apply first pass
    firstShaderTexture->BindFBO();
    RenderShader(firstShader, source, target);
    firstShaderTexture->UnbindFBO();

    // Apply all passes except the first and last one (which needs to be applied to the backbuffer)
    for (unsigned int shaderIdx = 1;
         shaderIdx < static_cast<unsigned int>(m_pShaders.size()) - 1;
         ++shaderIdx)
    {
      IShader* shader = m_pShaders[shaderIdx].get();
      CShaderTextureGL* prevTexture = m_pShaderTextures[shaderIdx - 1].get();
      CShaderTextureGL* texture = m_pShaderTextures[shaderIdx].get();
      texture->CreateFBO(screenWidth, screenHeight);
      texture->BindFBO();
      RenderShader(shader, prevTexture, target); // The target on each call is only used for setting the viewport
      texture->UnbindFBO();
    }
    // TODO: Remove last texture, useless
    // Apply last pass
    CShaderTextureGL* secToLastTexture = m_pShaderTextures[m_pShaderTextures.size() - 2].get();
    RenderShader(lastShader, secToLastTexture, target);
  }

  m_frameCount += static_cast<float>(m_speed);

  // Restore our view port.
  m_context.SetViewPort(viewPort);

  return true;
}


bool CShaderPresetGL::Update()
{
  auto updateFailed = [this](const std::string& msg)
  {
    m_failedPaths.insert(m_presetPath);
    auto message = "CShaderPresetGL::Update: " + msg + ". Disabling video shaders.";
    CLog::Log(LOGWARNING, message.c_str());
    DisposeShaders();
    return false;
  };

  if (m_bPresetNeedsUpdate && !HasPathFailed(m_presetPath))
  {
    DisposeShaders();

    if (m_presetPath.empty())
      return false;

    if (!ReadPresetFile(m_presetPath)) {
      CLog::Log(LOGERROR, "%s - couldn't load shader preset %s or the shaders it references", __func__, m_presetPath.c_str());
      return false;
    }

    if (!CreateShaders())
      return updateFailed("Failed to initialize shaders");

    if (!CreateBuffers())
      return updateFailed("Failed to initialize buffers");

    if (!CreateShaderTextures())
      return updateFailed("A shader texture failed to init");
  }

  if (m_pShaders.empty())
    return false;

  // Each pass must have its own texture and the opposite is also true
  if (m_pShaders.size() != m_pShaderTextures.size())
    return updateFailed("A shader or texture failed to init");

  m_bPresetNeedsUpdate = false;
  return true;
}

void CShaderPresetGL::SetVideoSize(const unsigned videoWidth, const unsigned videoHeight)
{
  m_videoSize = {videoWidth, videoHeight};
  m_textureSize = CShaderUtils::GetOptimalTextureSize(m_videoSize);
}

bool CShaderPresetGL::SetShaderPreset(const std::string &shaderPresetPath)
{
  m_bPresetNeedsUpdate = true;
  m_presetPath = shaderPresetPath;
  return Update();
}

const std::string &CShaderPresetGL::GetShaderPreset() const
{
  return m_presetPath;
}

bool CShaderPresetGL::CreateShaderTextures()
{
  m_pShaderTextures.clear();

  float2 prevSize = m_videoSize;

  unsigned int numPasses = static_cast<unsigned  int>(m_passes.size());

  for (unsigned shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    ShaderPass& pass = m_passes[shaderIdx];

    // resolve final texture resolution, taking scale type and scale multiplier into account
    float2 scaledSize;
    switch (pass.fbo.scaleX.type)
    {
      case SCALE_TYPE_ABSOLUTE:
        scaledSize.x = static_cast<float>(pass.fbo.scaleX.abs);
        break;
      case SCALE_TYPE_VIEWPORT:
        scaledSize.x = m_outputSize.x;
        break;
      case SCALE_TYPE_INPUT:
      default:
        scaledSize.x = prevSize.x;
        break;
    }
    switch (pass.fbo.scaleY.type)
    {
      case SCALE_TYPE_ABSOLUTE:
        scaledSize.y = static_cast<float>(pass.fbo.scaleY.abs);
        break;
      case SCALE_TYPE_VIEWPORT:
        scaledSize.y = m_outputSize.y;
        break;
      case SCALE_TYPE_INPUT:
      default:
        scaledSize.y = prevSize.y;
        break;
    }

    // if the scale was unspecified
    if (pass.fbo.scaleX.scale == 0 && pass.fbo.scaleY.scale == 0)
    {
      // if the last shader has the scale unspecified
      if (shaderIdx == numPasses - 1)
      {
        // we're supposed to output at full (viewport) res
        scaledSize.x = m_outputSize.x;
        scaledSize.y = m_outputSize.y;
      }
    }
    else
    {
      scaledSize.x *= pass.fbo.scaleX.scale;
      scaledSize.y *= pass.fbo.scaleY.scale;
    }

    // Determine the framebuffer data format
    unsigned int textureFormat;
    if (pass.fbo.floatFramebuffer)
    {
      // Give priority to float framebuffer parameter (we can't use both float and sRGB)
      textureFormat = GL_RGB32F;
    }
    else
    {
      if (pass.fbo.sRgbFramebuffer)
        textureFormat = GL_SRGB8;
      else
        textureFormat = GL_RGBA;
    }

    auto texture = new CGLTexture(
            static_cast<unsigned int>(scaledSize.x),
            static_cast<unsigned int>(scaledSize.y),
            textureFormat);

    texture->CreateTextureObject();

    if (texture->getMTexture() <= 0)
    {
      CLog::Log(LOGERROR, "Couldn't create a texture for video shader %s.", pass.sourcePath.c_str());
      return false;
    }

    glBindTexture(GL_TEXTURE_2D, texture->getMTexture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0.0);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, MAX_FLOAT);
    GLfloat blackBorder[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, blackBorder);

    m_pShaderTextures.emplace_back(new CShaderTextureGL(*texture));
    m_pShaders[shaderIdx]->SetSizes(prevSize, scaledSize);

    prevSize = scaledSize;

  }
  return true;
}

bool CShaderPresetGL::CreateShaders()
{
  auto numPasses = m_passes.size();
  m_textureSize = CShaderUtils::GetOptimalTextureSize(m_videoSize);
  ShaderLutVec passLUTsGL;
  for (unsigned shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const auto& pass = m_passes[shaderIdx];

    for (unsigned i = 0; i < pass.luts.size(); ++i)
    {
      auto& lutStruct = pass.luts[i];

      ShaderLutPtr passLut(new CShaderLutGL(lutStruct.strId, lutStruct.path));
      if (passLut->Create(m_context, lutStruct))
        passLUTsGL.emplace_back(std::move(passLut));
    }

    std::unique_ptr<CShaderGL> videoShader(new CShaderGL(m_context));

    auto shaderSource = pass.vertexSource; //also contains fragment source
    auto shaderPath = pass.sourcePath;

    // Get only the parameters belonging to this specific shader
    ShaderParameterMap passParameters = GetShaderParameters(pass.parameters, pass.vertexSource);

    if (!videoShader->Create(shaderSource, shaderPath, passParameters, nullptr, passLUTsGL, m_outputSize, pass.frameCountMod))
    {
      CLog::Log(LOGERROR, "Couldn't create a video shader");
      return false;
    }
    m_pShaders.push_back(std::move(videoShader));
  }
  return true;
}

bool CShaderPresetGL::CreateBuffers()
{
  for (auto& videoShader : m_pShaders)
  {
    videoShader->CreateInputBuffer();
  }
  return true;
}

void CShaderPresetGL::UpdateViewPort()
{
  CRect viewPort;
  m_context.GetViewPort(viewPort);
  UpdateViewPort(viewPort);
}

void CShaderPresetGL::UpdateViewPort(CRect viewPort)
{
  float2 currentViewPortSize = { viewPort.Width(), viewPort.Height() };
  if (currentViewPortSize != m_outputSize)
  {
    m_outputSize = currentViewPortSize;
    m_bPresetNeedsUpdate = true;
    Update();
  }
}

void CShaderPresetGL::UpdateMVPs()
{
  for (auto& videoShader : m_pShaders)
    videoShader->UpdateMVP();
}

void CShaderPresetGL::DisposeShaders()
{
  m_pShaders.clear();
  m_pShaderTextures.clear();
  m_passes.clear();
  m_bPresetNeedsUpdate = true;
}

void CShaderPresetGL::PrepareParameters(const IShaderTexture *texture, const CPoint *dest)
{
  for (unsigned shaderIdx = 0; shaderIdx < m_pShaders.size() - 1; ++shaderIdx)
  {
    auto& videoShader = m_pShaders[shaderIdx];
    videoShader->PrepareParameters(m_dest, false, static_cast<uint64_t>(m_frameCount));
  }

  m_pShaders.back()->PrepareParameters(m_dest, true, static_cast<uint64_t>(m_frameCount));

  if (m_dest[0] != dest[0] || m_dest[1] != dest[1]
      || m_dest[2] != dest[2] || m_dest[3] != dest[3]
      || texture->GetWidth() != m_outputSize.x
      || texture->GetHeight() != m_outputSize.y)
  {
    for (size_t i = 0; i < 4; ++i)
      m_dest[i] = dest[i];

    m_outputSize = { texture->GetWidth(), texture->GetHeight() };

    UpdateMVPs();
    UpdateViewPort();
  }
}

void CShaderPresetGL::RenderShader(IShader *shader, IShaderTexture *source, IShaderTexture *target) const
{
  CRect newViewPort(0.f, 0.f, target->GetWidth(), target->GetHeight());
  m_context.SetViewPort(newViewPort);
  m_context.SetScissors(newViewPort);

  shader->Render(source, target);
}

bool CShaderPresetGL::ReadPresetFile(const std::string &presetPath)
{
  return CServiceBroker::GetGameServices().VideoShaders().LoadPreset(presetPath, *this);
}

void CShaderPresetGL::SetSpeed(double speed)
{
  m_speed = speed;
}

ShaderPassVec &CShaderPresetGL::GetPasses()
{
  return m_passes;
}

bool CShaderPresetGL::HasPathFailed(const std::string &path) const
{
  return m_failedPaths.find(path) != m_failedPaths.end();
}
