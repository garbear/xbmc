/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetGL.h"

#include "ServiceBroker.h"
#include "ShaderUtilsGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "cores/RetroPlayer/shaders/ShaderUtils.h"
#include "cores/RetroPlayer/shaders/gl/ShaderGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderLutGL.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"

#include <regex>

#define MAX_FLOAT 3.402823466E+38

using namespace KODI;
using namespace SHADER;

CShaderPresetGL::CShaderPresetGL(RETRO::CRenderContext& context,
                                 unsigned int videoWidth,
                                 unsigned int videoHeight)
  : m_context(context), m_videoSize(videoWidth, videoHeight)
{
  m_textureSize = CShaderUtils::GetOptimalTextureSize(m_videoSize);

  CRect viewPort;
  m_context.GetViewPort(viewPort);
  m_outputSize = {viewPort.Width(), viewPort.Height()};
}

CShaderPresetGL::~CShaderPresetGL()
{
  DisposeShaders();

  // The GUI is going to render after this, so apply the state required
  m_context.ApplyStateBlock();
}

ShaderParameterMap CShaderPresetGL::GetShaderParameters(
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
    for (const auto& parameter : parameters)
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

bool CShaderPresetGL::RenderUpdate(const CPoint* dest,
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

  PrepareParameters(target, dest);

  IShader* firstShader = m_pShaders.front().get();
  IShader* lastShader = m_pShaders.back().get();

  const unsigned passesNum = static_cast<unsigned int>(m_pShaders.size());

  if (passesNum == 1)
    firstShader->Render(source, target);
  else if (passesNum == 2)
  {
    // Apply first pass
    CShaderTextureGL* firstShaderTexture = m_pShaderTextures.front().get();
    firstShaderTexture->BindFBO();
    RenderShader(firstShader, source, firstShaderTexture);
    firstShaderTexture->UnbindFBO();

    // Apply last pass
    CRect newViewPort(0.f, 0.f, target->GetWidth(), target->GetHeight());
    m_context.SetViewPort(newViewPort);
    m_context.SetScissors(newViewPort);
    lastShader->Render(firstShaderTexture, target);
  }
  else
  {
    // Apply first pass
    CShaderTextureGL* firstShaderTexture = m_pShaderTextures.front().get();
    firstShaderTexture->BindFBO();
    RenderShader(firstShader, source, firstShaderTexture);
    firstShaderTexture->UnbindFBO();

    // Apply all passes except the first and last one (which needs to be applied to the backbuffer)
    for (unsigned int shaderIdx = 1; shaderIdx < static_cast<unsigned int>(m_pShaders.size()) - 1;
         ++shaderIdx)
    {
      IShader* shader = m_pShaders[shaderIdx].get();
      CShaderTextureGL* prevTexture = m_pShaderTextures[shaderIdx - 1].get();
      CShaderTextureGL* texture = m_pShaderTextures[shaderIdx].get();
      texture->BindFBO();
      RenderShader(shader, prevTexture,
                   texture); // The target on each call is only used for setting the viewport
      texture->UnbindFBO();
    }

    // Apply last pass
    CShaderTextureGL* secToLastTexture = m_pShaderTextures[m_pShaderTextures.size() - 2].get();
    CRect newViewPort(0.f, 0.f, target->GetWidth(), target->GetHeight());
    m_context.SetViewPort(newViewPort);
    m_context.SetScissors(newViewPort);
    lastShader->Render(secToLastTexture, target);
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
    CLog::Log(LOGWARNING, "CShaderPresetGL::Update: {}. Disabling video shaders.", msg);
    DisposeShaders();
    return false;
  };

  if (m_bPresetNeedsUpdate && !HasPathFailed(m_presetPath))
  {
    DisposeShaders();

    if (m_presetPath.empty())
      return false;

    if (!ReadPresetFile(m_presetPath))
    {
      CLog::LogF(LOGERROR, "Couldn't load shader preset {} or the shaders it references",
                 m_presetPath);
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

  // Each pass except the last one must have its own texture and the opposite is also true
  if (m_pShaders.size() - 1 != m_pShaderTextures.size())
    return updateFailed("A shader or texture failed to init");

  m_bPresetNeedsUpdate = false;
  return true;
}

void CShaderPresetGL::SetVideoSize(const unsigned videoWidth, const unsigned videoHeight)
{
  if (videoWidth != m_videoSize.x || videoHeight != m_videoSize.y) {
    m_videoSize = {videoWidth, videoHeight};
    m_textureSize = CShaderUtils::GetOptimalTextureSize(m_videoSize);
    m_bPresetNeedsUpdate = true;
  }
}

bool CShaderPresetGL::SetShaderPreset(const std::string& shaderPresetPath)
{
  m_bPresetNeedsUpdate = true;
  m_presetPath = shaderPresetPath;
  return Update();
}

const std::string& CShaderPresetGL::GetShaderPreset() const
{
  return m_presetPath;
}

bool CShaderPresetGL::CreateShaderTextures()
{
  m_pShaderTextures.clear();

  unsigned int major, minor;
  CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);

  float2 prevSize = m_videoSize;

  unsigned int numPasses = static_cast<unsigned int>(m_passes.size());

  for (unsigned shaderIdx = 0; shaderIdx < numPasses - 1; ++shaderIdx)
  {
    ShaderPass& pass = m_passes[shaderIdx];
    ShaderPass& nextPass = m_passes[shaderIdx + 1];

    // Resolve final texture resolution, taking scale type and scale multiplier into account
    float2 scaledSize;
    switch (pass.fbo.scaleX.type)
    {
      case SCALE_TYPE_ABSOLUTE:
        scaledSize.x = static_cast<float>(pass.fbo.scaleX.abs);
        break;
      case SCALE_TYPE_VIEWPORT:
        scaledSize.x = m_outputSize.x * pass.fbo.scaleX.scale;
        break;
      case SCALE_TYPE_INPUT:
      default:
        scaledSize.x = prevSize.x * pass.fbo.scaleX.scale;
        break;
    }
    switch (pass.fbo.scaleY.type)
    {
      case SCALE_TYPE_ABSOLUTE:
        scaledSize.y = static_cast<float>(pass.fbo.scaleY.abs);
        break;
      case SCALE_TYPE_VIEWPORT:
        scaledSize.y = m_outputSize.y * pass.fbo.scaleY.scale;
        break;
      case SCALE_TYPE_INPUT:
      default:
        scaledSize.y = prevSize.y * pass.fbo.scaleY.scale;
        break;
    }

    // Determine the framebuffer data format
    GLint internalformat;
    GLenum pixelformat;
    if (pass.fbo.floatFramebuffer && major >= 3)
    {
      // Give priority to float framebuffer parameter (we can't use both float and sRGB)
      internalformat = GL_RGBA32F;
      pixelformat = GL_RGBA;
    }
    else
    {
      if (pass.fbo.sRgbFramebuffer) {
        internalformat = GL_SRGB8_ALPHA8;
        pixelformat = GL_RGBA;
      }
      else {
#ifndef HAS_GLES
        internalformat = GL_RGBA8;
        pixelformat = GL_BGRA;
#else
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif
#ifndef GL_BGRA8_EXT
#define GL_BGRA8_EXT 0x93A1
#endif
#ifdef TARGET_DARWIN
        internalformat = GL_RGBA;
#else
        internalformat = GL_BGRA_EXT;
#endif
        pixelformat = GL_BGRA_EXT;
#endif
      }
    }

    auto textureGL = new CGLTexture(static_cast<unsigned int>(scaledSize.x),
                                    static_cast<unsigned int>(scaledSize.y),
                                    XB_FMT_A8R8G8B8); // Format is not used

    textureGL->CreateTextureObject();

    if (textureGL->getMTexture() <= 0)
    {
      CLog::Log(LOGERROR, "Couldn't create a texture for video shader {}.", pass.sourcePath);
      return false;
    }

    auto wrapType = CShaderUtilsGL::TranslateWrapType(nextPass.wrap);
    auto magFilterType = (nextPass.filter == FILTER_TYPE_LINEAR ? GL_LINEAR : GL_NEAREST);
    auto minFilterType = (nextPass.mipmap ?
                          (nextPass.filter == FILTER_TYPE_LINEAR ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST) :
                          magFilterType);

    glBindTexture(GL_TEXTURE_2D, textureGL->getMTexture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilterType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapType);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapType);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0.0);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, MAX_FLOAT);
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, scaledSize.x, scaledSize.y, 0, pixelformat,
                 internalformat == GL_RGBA32F ? GL_FLOAT : GL_UNSIGNED_BYTE, (void*)0);

#ifndef HAS_GLES
    GLfloat blackBorder[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, blackBorder);
#endif

    m_pShaderTextures.emplace_back(new CShaderTextureGL(*textureGL));

    // Notify shader of its source and dest size
    m_pShaders[shaderIdx]->SetSizes(prevSize, scaledSize);

    prevSize = scaledSize;
  }

  // The last shader pass is supposed to output at full (viewport) resolution
  m_pShaders[numPasses - 1]->SetSizes(prevSize, m_outputSize);

  // Update MVPs
  UpdateMVPs();

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

    auto shaderSource = pass.vertexSource; // Also contains fragment source
    auto shaderPath = pass.sourcePath;

    // Get only the parameters belonging to this specific shader
    ShaderParameterMap passParameters = GetShaderParameters(pass.parameters, pass.vertexSource);

    if (!videoShader->Create(shaderSource, shaderPath, passParameters, nullptr, passLUTsGL,
                             m_outputSize, pass.frameCountMod))
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
    videoShader->CreateInputBuffer();

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
  float2 currentViewPortSize = {viewPort.Width(), viewPort.Height()};
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

void CShaderPresetGL::PrepareParameters(const IShaderTexture* texture, const CPoint* dest)
{
  if (m_dest[0] != dest[0] || m_dest[1] != dest[1] || m_dest[2] != dest[2] ||
      m_dest[3] != dest[3] || texture->GetWidth() != m_outputSize.x ||
      texture->GetHeight() != m_outputSize.y)
  {
    for (size_t i = 0; i < 4; ++i)
      m_dest[i] = dest[i];

    m_outputSize = {texture->GetWidth(), texture->GetHeight()};

    // Update projection matrix and update video shaders
    UpdateMVPs();
    UpdateViewPort();
  }

  // Prepare params for all shaders except the last (needs special flag)
  for (unsigned shaderIdx = 0; shaderIdx < m_pShaders.size() - 1; ++shaderIdx)
  {
    auto& videoShader = m_pShaders[shaderIdx];
    videoShader->PrepareParameters(m_dest, false, static_cast<uint64_t>(m_frameCount));
  }

  // Prepare params for last shader
  m_pShaders.back()->PrepareParameters(m_dest, true, static_cast<uint64_t>(m_frameCount));
}

void CShaderPresetGL::RenderShader(IShader* shader,
                                   IShaderTexture* source,
                                   IShaderTexture* target) const
{
  glViewport(0, 0, (GLsizei) target->GetWidth(), (GLsizei) target->GetHeight());
  glScissor(0, 0, (GLsizei) target->GetWidth(), (GLsizei) target->GetHeight());

  shader->Render(source, target);
}

bool CShaderPresetGL::ReadPresetFile(const std::string& presetPath)
{
  return CServiceBroker::GetGameServices().VideoShaders().LoadPreset(presetPath, *this);
}

void CShaderPresetGL::SetSpeed(double speed)
{
  m_speed = speed;
}

ShaderPassVec& CShaderPresetGL::GetPasses()
{
  return m_passes;
}

bool CShaderPresetGL::HasPathFailed(const std::string& path) const
{
  return m_failedPaths.find(path) != m_failedPaths.end();
}
