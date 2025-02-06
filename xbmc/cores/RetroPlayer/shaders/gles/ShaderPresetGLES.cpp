/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetGLES.h"

#include "ServiceBroker.h"
#include "ShaderGLES.h"
#include "ShaderLutGLES.h"
#include "ShaderUtilsGLES.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "cores/RetroPlayer/shaders/ShaderUtils.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"

#include <regex>

using namespace KODI;
using namespace SHADER;

CShaderPresetGLES::CShaderPresetGLES(RETRO::CRenderContext& context,
                                     unsigned int videoWidth,
                                     unsigned int videoHeight)
  : m_context(context), m_videoSize(videoWidth, videoHeight)
{
  CRect viewPort;
  m_context.GetViewPort(viewPort);
  m_outputSize = {viewPort.Width(), viewPort.Height()};
}

CShaderPresetGLES::~CShaderPresetGLES()
{
  DisposeShaders();

  // The GUI is going to render after this, so apply the state required
  m_context.ApplyStateBlock();
}

bool CShaderPresetGLES::ReadPresetFile(const std::string& presetPath)
{
  return CServiceBroker::GetGameServices().VideoShaders().LoadPreset(presetPath, *this);
}

bool CShaderPresetGLES::RenderUpdate(const CPoint dest[],
                                     IShaderTexture* source,
                                     IShaderTexture* target)
{
  // Save the viewport
  CRect viewPort;
  m_context.GetViewPort(viewPort);

  // Handle resizing of the viewport (window)
  UpdateViewPort(viewPort);

  // Update shaders/shader textures if required
  if (!Update())
    return false;

  PrepareParameters(dest, source, target);

  const unsigned int numPasses = static_cast<unsigned int>(m_pShaders.size());

  // Apply all passes except the last one (which needs to be applied to the backbuffer)
  for (unsigned int shaderIdx = 0; shaderIdx + 1 < numPasses; ++shaderIdx)
  {
    IShader* shader = m_pShaders[shaderIdx].get();
    IShaderTexture* texture = m_pShaderTextures[shaderIdx].get();
    RenderShader(shader, source, texture);
    source = texture;
  }

  // Restore our viewport
  m_context.SetViewPort(viewPort);
  m_context.SetScissors(viewPort);

  // Apply the last pass and write to target (backbuffer) instead of the last texture
  IShader* lastShader = m_pShaders.back().get();
  lastShader->Render(source, target);

  m_frameCount += static_cast<float>(m_speed);
  return true;
}

void CShaderPresetGLES::SetVideoSize(unsigned int videoWidth, unsigned int videoHeight)
{
  if (videoWidth != static_cast<unsigned int>(m_videoSize.x) ||
      videoHeight != static_cast<unsigned int>(m_videoSize.y))
  {
    m_videoSize = {videoWidth, videoHeight};
    m_bPresetNeedsUpdate = true;
  }
}

bool CShaderPresetGLES::SetShaderPreset(const std::string& shaderPresetPath)
{
  m_bPresetNeedsUpdate = true;
  m_presetPath = shaderPresetPath;
  return Update();
}

const std::string& CShaderPresetGLES::GetShaderPreset() const
{
  return m_presetPath;
}

bool CShaderPresetGLES::Update()
{
  auto updateFailed = [this](const std::string& msg)
  {
    m_failedPaths.insert(m_presetPath);
    CLog::Log(LOGWARNING, "CShaderPresetGLES::Update: {}", msg);
    DisposeShaders();
    return false;
  };

  if (m_bPresetNeedsUpdate && !HasPathFailed(m_presetPath))
  {
    DisposeShaders();

    if (m_presetPath.empty())
      // No preset should load, just return false, we shouldn't add "" to the failed paths
      return false;

    if (!ReadPresetFile(m_presetPath))
    {
      CLog::Log(
          LOGERROR,
          "CShaderPresetGLES::Update: Couldn't load shader preset {} or the shaders it references",
          m_presetPath);
      return false;
    }

    if (!CreateShaders())
      return updateFailed("Failed to initialize shaders");

    if (!CreateShaderTextures())
      return updateFailed("A shader texture failed to init");
  }

  if (m_pShaders.empty())
    return false;

  // Each pass except the last one must have its own texture and the opposite is also true
  if (m_pShaders.size() != m_pShaderTextures.size() + 1)
    return updateFailed("A shader or texture failed to init");

  m_bPresetNeedsUpdate = false;
  return true;
}

void CShaderPresetGLES::UpdateViewPort()
{
  CRect viewPort;
  m_context.GetViewPort(viewPort);
  UpdateViewPort(viewPort);
}

void CShaderPresetGLES::UpdateViewPort(CRect viewPort)
{
  float2 currentViewPortSize = {viewPort.Width(), viewPort.Height()};
  if (currentViewPortSize != m_outputSize)
  {
    m_outputSize = currentViewPortSize;
    m_bPresetNeedsUpdate = true;
    Update();
  }
}

void CShaderPresetGLES::UpdateMVPs()
{
  for (auto& videoShader : m_pShaders)
    videoShader->UpdateMVP();
}

void CShaderPresetGLES::PrepareParameters(const CPoint dest[],
                                          IShaderTexture* source,
                                          IShaderTexture* target)
{
  if (m_dest[0] != dest[0] || m_dest[1] != dest[1] || m_dest[2] != dest[2] ||
      m_dest[3] != dest[3] || target->GetWidth() != m_outputSize.x ||
      target->GetHeight() != m_outputSize.y)
  {
    for (size_t i = 0; i < 4; ++i)
      m_dest[i] = dest[i];

    m_outputSize = {target->GetWidth(), target->GetHeight()};

    // Update projection matrix and update video shaders
    UpdateMVPs();
    UpdateViewPort();
  }

  const unsigned int numPasses = static_cast<unsigned int>(m_pShaders.size());

  // Prepare parameters for all shader passes
  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    auto& videoShader = m_pShaders[shaderIdx];
    videoShader->PrepareParameters(m_dest, source, m_pShaderTextures, m_pShaders,
                                   static_cast<uint64_t>(m_frameCount));
  }
}

bool CShaderPresetGLES::CreateShaders()
{
  const unsigned int numPasses = static_cast<unsigned int>(m_passes.size());

  //! @todo Is this pass specific?
  std::vector<std::shared_ptr<IShaderLut>> passLUTsGL;
  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const ShaderPass& pass = m_passes[shaderIdx];
    const unsigned int numPassLuts = static_cast<unsigned int>(pass.luts.size());

    for (unsigned int i = 0; i < numPassLuts; ++i)
    {
      const ShaderLut& lutStruct = pass.luts[i];

      std::shared_ptr<CShaderLutGLES> passLut =
          std::make_shared<CShaderLutGLES>(lutStruct.strId, lutStruct.path);
      if (passLut->Create(m_context, lutStruct))
        passLUTsGL.emplace_back(std::move(passLut));
    }

    // Create the shader
    std::unique_ptr<CShaderGLES> videoShader = std::make_unique<CShaderGLES>(m_context);

    const std::string& shaderSource = pass.vertexSource; // Also contains fragment source
    const std::string& shaderPath = pass.sourcePath;

    // Get only the parameters belonging to this specific shader
    ShaderParameterMap passParameters = GetShaderParameters(pass.parameters, pass.vertexSource);

    if (!videoShader->Create(shaderSource, shaderPath, passParameters, passLUTsGL, m_outputSize,
                             shaderIdx, pass.frameCountMod))
    {
      CLog::Log(LOGERROR, "CShaderPresetGLES::CreateShaders: Couldn't create a video shader");
      return false;
    }
    m_pShaders.push_back(std::move(videoShader));
  }

  return true;
}

bool CShaderPresetGLES::CreateShaderTextures()
{
  m_pShaderTextures.clear();

  unsigned int major, minor;
  CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);

  float2 prevSize = m_videoSize;
  float2 prevTextureSize = m_videoSize;

  const unsigned int numPasses = static_cast<unsigned int>(m_passes.size());

  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const auto& pass = m_passes[shaderIdx];

    // Resolve final texture resolution, taking scale type and scale multiplier into account
    float2 scaledSize, textureSize;
    switch (pass.fbo.scaleX.scaleType)
    {
      case ScaleType::ABSOLUTE_SCALE:
        scaledSize.x = static_cast<float>(pass.fbo.scaleX.abs);
        break;
      case ScaleType::VIEWPORT:
        scaledSize.x =
            pass.fbo.scaleX.scale ? pass.fbo.scaleX.scale * m_outputSize.x : m_outputSize.x;
        break;
      case ScaleType::INPUT:
      default:
        scaledSize.x = pass.fbo.scaleX.scale ? pass.fbo.scaleX.scale * prevSize.x : prevSize.x;
        break;
    }
    switch (pass.fbo.scaleY.scaleType)
    {
      case ScaleType::ABSOLUTE_SCALE:
        scaledSize.y = static_cast<float>(pass.fbo.scaleY.abs);
        break;
      case ScaleType::VIEWPORT:
        scaledSize.y =
            pass.fbo.scaleY.scale ? pass.fbo.scaleY.scale * m_outputSize.y : m_outputSize.y;
        break;
      case ScaleType::INPUT:
      default:
        scaledSize.y = pass.fbo.scaleY.scale ? pass.fbo.scaleY.scale * prevSize.y : prevSize.y;
        break;
    }

    if (shaderIdx + 1 == numPasses)
    {
      // We're supposed to output at full (viewport) resolution
      scaledSize.x = m_outputSize.x;
      scaledSize.y = m_outputSize.y;
    }
    else
    {
      // Determine the framebuffer data format
      GLint internalFormat;
      GLenum pixelFormat;
      if (pass.fbo.floatFramebuffer && major >= 3)
      {
        // Give priority to float framebuffer parameter (we can't use both float and sRGB)
        internalFormat = GL_RGBA32F;
        pixelFormat = GL_RGBA;
      }
      else
      {
        if (pass.fbo.sRgbFramebuffer && major >= 3)
        {
          internalFormat = GL_SRGB8_ALPHA8;
          pixelFormat = GL_RGBA;
        }
        else
        {
          internalFormat = GL_RGBA;
          pixelFormat = GL_RGBA;
        }
      }

      //! @todo Enable usage of optimal texture sizes when all issues are fixed
      textureSize = scaledSize; // CShaderUtils::GetOptimalTextureSize(scaledSize)

      CGLESTexture* textureGL = new CGLESTexture(static_cast<unsigned int>(textureSize.x),
                                                 static_cast<unsigned int>(textureSize.y),
                                                 XB_FMT_A8R8G8B8); // Format is not used

      textureGL->CreateTextureObject();

      if (textureGL->getMTexture() <= 0)
      {
        CLog::Log(
            LOGERROR,
            "CShaderPresetGLES::CreateShaderTextures: Couldn't create texture for video shader: {}",
            pass.sourcePath);
        return false;
      }

      ShaderPass& nextPass = m_passes[shaderIdx + 1];

      if (nextPass.mipmap)
        textureGL->SetMipmapping();

      textureGL->SetScalingMethod(nextPass.filterType == FilterType::LINEAR
                                      ? TEXTURE_SCALING::LINEAR
                                      : TEXTURE_SCALING::NEAREST);

      const GLint wrapType = CShaderUtilsGLES::TranslateWrapType(nextPass.wrapType);
      const GLuint magFilterType =
          (nextPass.filterType == FilterType::LINEAR ? GL_LINEAR : GL_NEAREST);
      const GLuint minFilterType =
          (nextPass.mipmap ? (nextPass.filterType == FilterType::LINEAR ? GL_LINEAR_MIPMAP_LINEAR
                                                                        : GL_NEAREST_MIPMAP_NEAREST)
                           : magFilterType);

      glBindTexture(GL_TEXTURE_2D, textureGL->getMTexture());
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilterType);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterType);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapType);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapType);
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, textureSize.x, textureSize.y, 0, pixelFormat,
                   internalFormat == GL_RGBA32F ? GL_FLOAT : GL_UNSIGNED_BYTE, (void*)0);

      m_pShaderTextures.emplace_back(new CShaderTextureGLES(*textureGL, pass.fbo.sRgbFramebuffer));
    }

    // Notify shader of its source and dest size
    m_pShaders[shaderIdx]->SetSizes(prevSize, prevTextureSize, scaledSize);

    prevSize = scaledSize;
    prevTextureSize = textureSize;
  }

  UpdateMVPs();
  return true;
}

void CShaderPresetGLES::RenderShader(IShader* shader,
                                     IShaderTexture* source,
                                     IShaderTexture* target) const
{
  if (static_cast<CShaderTextureGLES*>(target)->BindFBO())
  {
    const CRect newViewPort(0.f, 0.f, target->GetWidth(), target->GetHeight());
    glViewport((GLsizei)newViewPort.x1, (GLsizei)newViewPort.y1, (GLsizei)newViewPort.x2,
               (GLsizei)newViewPort.y2);
    glScissor((GLsizei)newViewPort.x1, (GLsizei)newViewPort.y1, (GLsizei)newViewPort.x2,
              (GLsizei)newViewPort.y2);

    shader->Render(source, target);
    static_cast<CShaderTextureGLES*>(target)->UnbindFBO();
  }
}

void CShaderPresetGLES::DisposeShaders()
{
  m_pShaders.clear();
  m_pShaderTextures.clear();
  m_passes.clear();
  m_bPresetNeedsUpdate = true;
}

bool CShaderPresetGLES::HasPathFailed(const std::string& path) const
{
  return m_failedPaths.find(path) != m_failedPaths.end();
}

ShaderParameterMap CShaderPresetGLES::GetShaderParameters(
    const std::vector<ShaderParameter>& parameters, const std::string& sourceStr) const
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

  // For each param found in the source code
  for (const auto& match : validParams)
  {
    // For each param found in the preset file
    for (const ShaderParameter& parameter : parameters)
    {
      // Check if they match
      if (match == parameter.strId)
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
