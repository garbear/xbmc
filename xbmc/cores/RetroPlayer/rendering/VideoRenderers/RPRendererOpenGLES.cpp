/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererOpenGLES.h"

#include "RPHdrShaderGLES.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/buffers/RenderBufferOpenGLES.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolOpenGLES.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/gles/ShaderPresetGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLESRef.h"
#include "rendering/GLExtensions.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <cstddef>

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryOpenGLES ------------------------------------------------

std::string CRendererFactoryOpenGLES::RenderSystemName() const
{
  return "OpenGLES";
}

CRPBaseRenderer* CRendererFactoryOpenGLES::CreateRenderer(
    const CRenderSettings& settings,
    CRenderContext& context,
    std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererOpenGLES(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryOpenGLES::CreateBufferPools(CRenderContext& context)
{
  bool supportsTextureSwizzle = false;
#if defined(GL_ES_VERSION_3_0)
  auto* renderSystem = static_cast<CRenderSystemGLES*>(context.Rendering());
  supportsTextureSwizzle = renderSystem && renderSystem->SupportsTextureSwizzle();
#endif
  return {std::make_shared<CRenderBufferPoolOpenGLES>(supportsTextureSwizzle)};
}

// --- CRPRendererOpenGLES -----------------------------------------------------

CRPRendererOpenGLES::CRPRendererOpenGLES(const CRenderSettings& renderSettings,
                                         CRenderContext& context,
                                         std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
  m_context.CaptureStateBlock();

  // Initialize CRPBaseRenderer
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGLES>(m_context);

  const GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip

  // Set up main screen VBO
  glGenBuffers(1, &m_mainVertexVBO);

  glGenBuffers(1, &m_mainIndexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mainIndexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 4, idx, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  m_context.ApplyStateBlock();
}

CRPRendererOpenGLES::~CRPRendererOpenGLES()
{
  glDeleteBuffers(1, &m_mainIndexVBO);
  glDeleteBuffers(1, &m_mainVertexVBO);
}

void CRPRendererOpenGLES::RenderInternal(uint8_t alpha)
{
  Render(alpha);
}

void CRPRendererOpenGLES::FlushInternal()
{
  m_RBTexturesMap.clear();

  if (!m_bConfigured)
    return;

  glFinish();
}

bool CRPRendererOpenGLES::Supports(RENDERFEATURE feature) const
{
  return feature == RENDERFEATURE::STRETCH || feature == RENDERFEATURE::ZOOM ||
         feature == RENDERFEATURE::PIXEL_RATIO || feature == RENDERFEATURE::ROTATION;
}

bool CRPRendererOpenGLES::SupportsScalingMethod(SCALINGMETHOD method)
{
  return method == SCALINGMETHOD::AUTO || method == SCALINGMETHOD::NEAREST ||
         method == SCALINGMETHOD::LINEAR;
}

void CRPRendererOpenGLES::Render(uint8_t alpha)
{
  auto renderBuffer = static_cast<CRenderBufferOpenGLES*>(m_renderBuffer);
  if (renderBuffer == nullptr)
    return;

  UpdateShaders();

  // Use video shader preset
  if (m_bUseShaderPreset)
  {
    RenderBufferTextures* rbTextures;

    // Drop cached textures if target size is changed
    if (m_fullDestWidth != m_lastTargetWidth || m_fullDestHeight != m_lastTargetHeight)
    {
      m_RBTexturesMap.clear();
      m_lastTargetWidth = m_fullDestWidth;
      m_lastTargetHeight = m_fullDestHeight;
    }

    const auto it = m_RBTexturesMap.find(renderBuffer);
    if (it != m_RBTexturesMap.end() &&
        it->second->sourceTexture->GetTextureID() == renderBuffer->TextureID() &&
        it->second->sourceTexture->GetWidth() == renderBuffer->GetWidth() &&
        it->second->sourceTexture->GetHeight() == renderBuffer->GetHeight())
    {
      rbTextures = it->second.get();
    }
    else
    {
      if (it != m_RBTexturesMap.end())
        m_RBTexturesMap.erase(it);

      rbTextures = new RenderBufferTextures{
          // Source texture
          std::make_shared<SHADER::CShaderTextureGLESRef>(
              static_cast<unsigned int>(renderBuffer->GetWidth()),
              static_cast<unsigned int>(renderBuffer->GetHeight()), renderBuffer->TextureID()),
          // Target texture
          std::make_shared<SHADER::CShaderTextureGLES>(static_cast<unsigned int>(m_fullDestWidth),
                                                       static_cast<unsigned int>(m_fullDestHeight),
                                                       GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, false),
          // HDR target texture, made when first needed
          nullptr};
      rbTextures->targetTexture->CreateTexture(); // Create new internal texture
      m_RBTexturesMap.emplace(renderBuffer, rbTextures);
    }

    std::shared_ptr<SHADER::CShaderTextureGLESRef> sourceTexture = rbTextures->sourceTexture;
    SHADER::CShaderTextureGLES& targetTexture =
        GetTargetTexture(*rbTextures, {m_fullDestWidth, m_fullDestHeight});

    GLint filter = GL_NEAREST;
    if (m_shaderPreset->GetPasses().front().filterType == SHADER::FilterType::LINEAR)
      filter = GL_LINEAR;

    glBindTexture(m_textureTarget, sourceTexture->GetTextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (m_shaderPreset->RenderUpdate(*sourceTexture, targetTexture))
    {
      glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
      glBindTexture(m_textureTarget, targetTexture.GetTextureID());
    }
    else
    {
      m_bShadersNeedUpdate = false;
      m_bUseShaderPreset = false;
    }
  }

  if (!m_bUseShaderPreset)
  {
    GLint filter = GL_NEAREST;
    if (GetRenderSettings().VideoSettings().GetScalingMethod() == SCALINGMETHOD::LINEAR)
      filter = GL_LINEAR;

    glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
    glBindTexture(m_textureTarget, renderBuffer->TextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  DrawFrame(alpha, renderBuffer->GetWidth(), renderBuffer->GetHeight());
}

SHADER::CShaderTextureGLES& CRPRendererOpenGLES::GetTargetTexture(RenderBufferTextures& textures,
                                                                  const CSize& size)
{
  if (m_hdrPeak == 0)
    return *textures.targetTexture;

  // More than 8 bits, as the HDR shader raises the brightest values steeply
  if (!textures.hdrTargetTexture)
  {
    GLuint pixelType = GL_UNSIGNED_BYTE;
    GLuint internalFormat = GL_RGBA;
#if HAS_GLES >= 3
    // A GLES 3 context can always render to 10 bits, but to half floats only
    // with an extension. GBM can fall back to a GLES 2 context.
    unsigned int major{0};
    unsigned int minor{0};
    CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
    if (major >= 3)
    {
      if (CGLExtensions::IsExtensionSupported(CGLExtensions::EXT_color_buffer_float))
      {
        pixelType = GL_HALF_FLOAT;
        internalFormat = GL_RGBA16F;
      }
      else
      {
        pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
        internalFormat = GL_RGB10_A2;
      }
    }
#endif
    textures.hdrTargetTexture = std::make_shared<SHADER::CShaderTextureGLES>(
        static_cast<unsigned int>(size.Width()), static_cast<unsigned int>(size.Height()),
        pixelType, internalFormat, GL_RGBA, false);
    textures.hdrTargetTexture->CreateTexture();
  }

  return *textures.hdrTargetTexture;
}

bool CRPRendererOpenGLES::LoadHdrShader()
{
  if (m_hdrShader)
    return true;

  if (m_hdrShaderFailed)
    return false;

  std::string defines;
  if (m_context.UseLimitedColor())
    defines += "#define KODI_LIMITED_RANGE 1\n";

  auto shader = std::make_unique<CRPHdrShaderGLES>(defines);
  if (!shader->CompileAndLink())
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to compile the HDR shader");
    m_hdrShaderFailed = true;
    return false;
  }

  m_hdrShader = std::move(shader);
  return true;
}

void CRPRendererOpenGLES::DrawFrame(uint8_t alpha, float width, float height)
{
  // A frame's own alpha means nothing, and an XRGB core or a video filter can
  // leave it at zero. Over HDR, Kodi lays the GUI on the picture by the GUI's
  // alpha, so a zero let the picture through where a game was drawn in the GUI.
  // A fade is blended in instead.
  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendColor(0.0f, 0.0f, 0.0f, alpha / 255.0f);
    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  // Setup texture coordinates
  CRect rect = m_sourceRect;
  rect.x1 /= width;
  rect.x2 /= width;
  rect.y1 /= height;
  rect.y2 /= height;

  if (m_hdrPeak != 0 && LoadHdrShader())
  {
    m_hdrShader->SetLuminance(m_hdrPaperWhite, m_hdrPeak);
    m_hdrShader->SetQuad(m_rotatedDestCoords[0], m_rotatedDestCoords[1], m_rotatedDestCoords[3],
                         rect);
    m_hdrShader->Enable();

    const GLfloat quad[] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    glBindBuffer(GL_ARRAY_BUFFER, m_mainVertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);
    const GLint posLoc = m_hdrShader->GetPosLoc();
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(posLoc);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(posLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_hdrShader->Disable();
    return;
  }

  // Use GUI shader
  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE_NOALPHA);

  GLint posLoc = m_context.GUIShaderGetPos();
  GLint tex0Loc = m_context.GUIShaderGetCoord0();
  GLint uniColLoc = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

  // Setup color values
  GLubyte col[4];
  const uint32_t color = (alpha << 24) | 0xFFFFFF;
  col[0] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::R, color);
  col[1] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::G, color);
  col[2] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::B, color);
  col[3] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::A, color);

  glUniform4f(uniColLoc, (col[0] / 255.0f), (col[1] / 255.0f), (col[2] / 255.0f),
              (col[3] / 255.0f));
  glUniform1f(depthLoc, -1.0f);

  PackedVertex vertex[4];

  // Setup vertex position values
  for (unsigned int i = 0; i < 4; i++)
  {
    vertex[i].x = m_rotatedDestCoords[i].x;
    vertex[i].y = m_rotatedDestCoords[i].y;
    vertex[i].z = 0.0f;
  }

  // Setup texture coordinates
  vertex[0].u1 = vertex[3].u1 = rect.x1;
  vertex[0].v1 = vertex[1].v1 = rect.y1;
  vertex[1].u1 = vertex[2].u1 = rect.x2;
  vertex[2].v1 = vertex[3].v1 = rect.y2;

  glBindBuffer(GL_ARRAY_BUFFER, m_mainVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex) * 4, &vertex[0], GL_DYNAMIC_DRAW);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glEnableVertexAttribArray(posLoc);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));
  glEnableVertexAttribArray(tex0Loc);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mainIndexVBO);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, nullptr);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();
}
