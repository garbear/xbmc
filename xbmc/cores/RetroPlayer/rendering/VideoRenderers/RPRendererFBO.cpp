/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererFBO.h"

#include "cores/RetroPlayer/buffers/RenderBufferFBO.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolFBO.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#if defined(HAS_GLES)
#include "cores/RetroPlayer/shaders/gles/ShaderPresetGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLESRef.h"
#else
#include "cores/RetroPlayer/shaders/gl/ShaderPresetGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGLRef.h"
#endif
#include "rendering/MatrixGL.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <stddef.h>

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryFBO ------------------------------------------------

std::string CRendererFactoryFBO::RenderSystemName() const
{
  return "FBO";
}

CRPBaseRenderer* CRendererFactoryFBO::CreateRenderer(const CRenderSettings& settings,
                                                     CRenderContext& context,
                                                     std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererFBO(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryFBO::CreateBufferPools(CRenderContext& context)
{
  return {std::make_shared<CRenderBufferPoolFBO>(context)};
}

// --- CRPRendererFBO -----------------------------------------------------

CRPRendererFBO::CRPRendererFBO(const CRenderSettings& renderSettings,
                               CRenderContext& context,
                               std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
  // Without this the video filter settings have nothing behind them and are
  // silently ignored, which is what every hardware-rendered game got before.
#if defined(HAS_GLES)
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGLES>(m_context);
#else
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGL>(m_context);
#endif
}

CRPRendererFBO::~CRPRendererFBO()
{
#if !defined(HAS_GLES)
  if (m_vao != 0)
  {
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
  }
#endif

  DestroyShaderResources();
}

void CRPRendererFBO::DestroyShaderResources()
{
  if (m_shaderCopyFbo != 0)
  {
    glDeleteFramebuffers(1, &m_shaderCopyFbo);
    m_shaderCopyFbo = 0;
  }

  if (m_shaderSourceTexture != 0)
  {
    glDeleteTextures(1, &m_shaderSourceTexture);
    m_shaderSourceTexture = 0;
  }

  m_shaderSourceWidth = 0;
  m_shaderSourceHeight = 0;

  m_shaderTargetTexture.reset();
  m_shaderTargetWidth = 0.0f;
  m_shaderTargetHeight = 0.0f;
}

bool CRPRendererFBO::CopyFrameForShaders(CRenderBufferFBO* renderBuffer)
{
  const unsigned int frameWidth = renderBuffer->GetWidth();
  const unsigned int frameHeight = renderBuffer->GetHeight();

  if (frameWidth == 0 || frameHeight == 0)
    return false;

  // Rebuilt only when the frame changes size, which for most clients is never
  if (m_shaderSourceTexture == 0 || m_shaderSourceWidth != frameWidth ||
      m_shaderSourceHeight != frameHeight)
  {
    if (m_shaderSourceTexture != 0)
      glDeleteTextures(1, &m_shaderSourceTexture);

    glGenTextures(1, &m_shaderSourceTexture);
    glBindTexture(m_textureTarget, m_shaderSourceTexture);
    glTexImage2D(m_textureTarget, 0, GL_RGBA, frameWidth, frameHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(m_textureTarget, 0);

    m_shaderSourceWidth = frameWidth;
    m_shaderSourceHeight = frameHeight;

    if (m_shaderCopyFbo == 0)
      glGenFramebuffers(1, &m_shaderCopyFbo);

    glBindFramebuffer(GL_FRAMEBUFFER, m_shaderCopyFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textureTarget,
                           m_shaderSourceTexture, 0);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
      CLog::Log(LOGERROR,
                "RetroPlayer[RENDER]: Can't shade a {}x{} frame, its copy is incomplete ({:#x})",
                frameWidth, frameHeight, status);
      DestroyShaderResources();
      return false;
    }
  }

  // The client drew into a corner of its framebuffer, so only that corner is
  // copied. Its origin follows the client's convention, the same one the
  // direct path flips for when sampling.
  // Flip a bottom-up client while copying, so the chain is always handed a
  // texture the conventional way up. Passing the flip on to the end instead
  // would leave the filters themselves running upside down, which matters for
  // any of them that is not symmetrical: scanlines, curvature, borders.
  const GLint srcY0 = renderBuffer->BottomLeftOrigin() ? frameHeight : 0;
  const GLint srcY1 = renderBuffer->BottomLeftOrigin() ? 0 : frameHeight;

  glBindFramebuffer(GL_READ_FRAMEBUFFER, renderBuffer->GetCurrentFramebuffer());
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_shaderCopyFbo);
  glBlitFramebuffer(0, srcY0, frameWidth, srcY1, 0, 0, frameWidth, frameHeight,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  return true;
}

void CRPRendererFBO::RenderInternal(bool clear, uint8_t alpha)
{

  if (clear)
  {
    if (alpha == 255)
      DrawBlackBars();
    else
      ClearBackBuffer();
  }

  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  Render(alpha);

  glEnable(GL_BLEND);
  glFlush();

  m_context.ApplyStateBlock();
}

void CRPRendererFBO::FlushInternal()
{
  if (!m_bConfigured)
    return;

  glFinish();
}

bool CRPRendererFBO::Supports(RENDERFEATURE feature) const
{
  return feature == RENDERFEATURE::STRETCH || feature == RENDERFEATURE::ZOOM ||
         feature == RENDERFEATURE::PIXEL_RATIO || feature == RENDERFEATURE::ROTATION;
}

bool CRPRendererFBO::SupportsScalingMethod(SCALINGMETHOD method)
{
  return method == SCALINGMETHOD::AUTO || method == SCALINGMETHOD::NEAREST ||
         method == SCALINGMETHOD::LINEAR;
}

void CRPRendererFBO::ClearBackBuffer()
{
  glClearColor(m_clearColour, m_clearColour, m_clearColour, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void CRPRendererFBO::DrawBlackBars()
{
  glDisable(GL_BLEND);

  struct Svertex
  {
    float x;
    float y;
    float z;
  };
  Svertex vertices[24];
  GLubyte count = 0;

  m_context.EnableGUIShader(GL_SHADER_METHOD::DEFAULT);
  GLint posLoc = m_context.GUIShaderGetPos();
  GLint uniCol = m_context.GUIShaderGetUniCol();

  glUniform4f(uniCol, m_clearColour / 255.0f, m_clearColour / 255.0f, m_clearColour / 255.0f, 1.0f);

  // top quad
  if (m_rotatedDestCoords[0].y > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = 0.0;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = 0;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = m_rotatedDestCoords[0].y;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = m_rotatedDestCoords[0].y;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  // bottom quad
  if (m_rotatedDestCoords[2].y < m_context.GetScreenHeight())
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_rotatedDestCoords[2].y;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = m_rotatedDestCoords[2].y;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = m_context.GetScreenHeight();
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = m_context.GetScreenHeight();
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  // left quad
  if (m_rotatedDestCoords[0].x > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_rotatedDestCoords[0].y;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_rotatedDestCoords[0].x;
    vertices[quad + 1].y = m_rotatedDestCoords[0].y;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_rotatedDestCoords[3].x;
    vertices[quad + 2].y = m_rotatedDestCoords[3].y;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = m_rotatedDestCoords[3].y;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  // right quad
  if (m_rotatedDestCoords[2].x < m_context.GetScreenWidth())
  {
    GLubyte quad = count;
    vertices[quad].x = m_rotatedDestCoords[1].x;
    vertices[quad].y = m_rotatedDestCoords[1].y;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = m_rotatedDestCoords[1].y;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = m_rotatedDestCoords[2].y;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = m_rotatedDestCoords[1].x;
    vertices[quad + 4].y = m_rotatedDestCoords[2].y;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

#if !defined(HAS_GLES)
  // A core profile has no default vertex array object, so drawing without one
  // bound is GL_INVALID_OPERATION and nothing reaches the screen. GLES permits
  // the default object, which is why this renderer worked there and showed a
  // black picture on desktop GL. Created once and reused.
  if (m_vao == 0)
    glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);
#endif

  GLuint vertexVBO;
  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Svertex) * count, &vertices[0], GL_STATIC_DRAW);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Svertex), 0);
  glEnableVertexAttribArray(posLoc);

  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(posLoc);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);

#if !defined(HAS_GLES)
  glBindVertexArray(0);
#endif

  m_context.DisableGUIShader();
}

void CRPRendererFBO::Render(uint8_t alpha)
{
  CRenderBufferFBO* renderBuffer = static_cast<CRenderBufferFBO*>(m_renderBuffer);

  if (renderBuffer == nullptr)
    return;

  CRect rect = m_sourceRect;

  // The source rect covers the frame the client drew, which is usually a corner
  // of a larger texture, so it is the texture that these are measured against.
  // Dividing by the frame size instead would stretch that corner over the whole
  // image, showing the unwritten remainder of the framebuffer with it.
  rect.x1 /= renderBuffer->TextureWidth();
  rect.x2 /= renderBuffer->TextureWidth();
  rect.y1 /= renderBuffer->TextureHeight();
  rect.y2 /= renderBuffer->TextureHeight();

  // The quad is placed in Kodi's screen coordinates, which grow downwards, while
  // a client rendering with OpenGL's convention has written its framebuffer from
  // the bottom up. Flip the texture for those clients so the image is sampled
  // the way round it was drawn.
  //
  // This tests the origin the opposite way to how it reads: while this renderer
  // set up its own Ortho2D with y growing upwards, the quad itself was already
  // inverted and it was the top-left origin clients that needed correcting. Now
  // that the render context's matrices place the quad, that inversion is gone.
  if (renderBuffer->BottomLeftOrigin())
    std::swap(rect.y1, rect.y2);

  // Logged once per stream: what the client said it drew, against the texture
  // it drew into and the rect that is sampled from it. A client that reports a
  // frame smaller than it really drew shows only part of its image, and there
  // is otherwise nothing in the log to tell that apart from a client that drew
  // only part of its framebuffer.
  // Logged whenever it changes rather than once, because a geometry that keeps
  // changing looks the same in a one-shot log as one that never does, and the
  // difference is the whole question when a picture flickers. A geometry that
  // changes every frame would drown the log, so lines are kept to one a second
  // and carry the number of changes they stand for.
  const FrameGeometry geometry{renderBuffer->GetWidth(),
                               renderBuffer->GetHeight(),
                               renderBuffer->TextureWidth(),
                               renderBuffer->TextureHeight(),
                               m_sourceRect,
                               rect,
                               renderBuffer->BottomLeftOrigin()};

  if (!m_bLoggedGeometry || geometry != m_loggedGeometry)
  {
    ++m_geometryChanges;

    const auto now = std::chrono::steady_clock::now();
    const bool bQuietEnough =
        !m_bLoggedGeometry ||
        (now - m_lastGeometryLog) >= std::chrono::seconds(1);

    if (bQuietEnough)
    {
      CLog::Log(LOGINFO,
                "RetroPlayer[RENDER]: FBO geometry: frame {}x{}, texture {}x{}, source rect "
                "({:.1f},{:.1f})-({:.1f},{:.1f}), sampling ({:.3f},{:.3f})-({:.3f},{:.3f}), "
                "bottom-left origin {} ({} change(s))",
                geometry.frameWidth, geometry.frameHeight, geometry.textureWidth,
                geometry.textureHeight, geometry.sourceRect.x1, geometry.sourceRect.y1,
                geometry.sourceRect.x2, geometry.sourceRect.y2, geometry.samplingRect.x1,
                geometry.samplingRect.y1, geometry.samplingRect.x2, geometry.samplingRect.y2,
                geometry.bottomLeftOrigin ? "yes" : "no", m_geometryChanges);

      m_bLoggedGeometry = true;
      m_lastGeometryLog = now;
      m_geometryChanges = 0;
    }

    m_loggedGeometry = geometry;
  }

  const uint32_t color = (alpha << 24) | 0xFFFFFF;

  // Run the video filter, if one is set. The chain reads a texture holding just
  // the client's frame and writes one the size of the destination, which is
  // then drawn in place of the client's own texture. Any failure along the way
  // turns the preset off and leaves the direct path below to draw the frame as
  // it always did, so a filter that cannot run costs the picture nothing.
  // Order our reads after the client's writes. Without this the client's last
  // draw calls may not have landed, and what is missing is whatever it drew
  // last -- typically the HUD.
  if (auto* fboPool = dynamic_cast<CRenderBufferPoolFBO*>(GetBufferPool()))
    fboPool->WaitForClientFrame();

  GLuint drawTexture = renderBuffer->TextureID();
  bool bShaded = false;

  Updateshaders();

  // Say why the filter is not running. Without this the silent cases -- no
  // preset reaching these render settings, or a preset that loaded with no
  // passes -- look identical to a filter that ran and did nothing.
  {
    const std::string& presetPath = m_renderSettings.VideoSettings().GetShaderPreset();
    const size_t passCount = m_shaderPreset ? m_shaderPreset->GetPasses().size() : 0;
    if (presetPath != m_lastLoggedPreset || m_bUseShaderPreset != m_bLastLoggedUsePreset)
    {
      CLog::Log(LOGINFO, "RetroPlayer[RENDER]: Video filter is \"{}\", in use {}, {} passes",
                presetPath.empty() ? "<none>" : presetPath, m_bUseShaderPreset, passCount);
      m_lastLoggedPreset = presetPath;
      m_bLastLoggedUsePreset = m_bUseShaderPreset;
    }
  }

  if (m_bUseShaderPreset && !m_shaderPreset->GetPasses().empty())
  {
    // Size the chain's target to what is actually being drawn, not to
    // m_fullDestWidth/Height. Those describe the picture at fullscreen, so a
    // game drawn into a GUI control -- every preview in the video filter dialog
    // -- had its filter rendered at 1440x1080 and then scaled down into a
    // thumbnail. Anything whose effect lives at the pixel level does not
    // survive that: scanlines and dot matrices vanish while overlays come
    // through, which is exactly the split seen in practice.
    const float destWidth = std::hypot(m_rotatedDestCoords[1].x - m_rotatedDestCoords[0].x,
                                       m_rotatedDestCoords[1].y - m_rotatedDestCoords[0].y);
    const float destHeight = std::hypot(m_rotatedDestCoords[2].x - m_rotatedDestCoords[1].x,
                                        m_rotatedDestCoords[2].y - m_rotatedDestCoords[1].y);

    if (m_shaderTargetTexture &&
        (m_shaderTargetWidth != destWidth || m_shaderTargetHeight != destHeight))
    {
      m_shaderTargetTexture.reset();
    }

    if (!m_shaderTargetTexture && destWidth > 0.0f && destHeight > 0.0f)
    {
#if defined(HAS_GLES)
      auto targetTexture = std::make_shared<SHADER::CShaderTextureGLES>(
          static_cast<unsigned int>(destWidth), static_cast<unsigned int>(destHeight),
          GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, false);
#else
      auto targetTexture = std::make_shared<SHADER::CShaderTextureGL>(
          static_cast<unsigned int>(destWidth), static_cast<unsigned int>(destHeight),
          GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, false);
#endif
      targetTexture->CreateTexture();
      m_shaderTargetTexture = std::move(targetTexture);
      m_shaderTargetWidth = destWidth;
      m_shaderTargetHeight = destHeight;
    }

    if (m_shaderTargetTexture && CopyFrameForShaders(renderBuffer))
    {
#if defined(HAS_GLES)
      SHADER::CShaderTextureGLESRef sourceTexture(m_shaderSourceWidth, m_shaderSourceHeight,
                                                  m_shaderSourceTexture);
      auto* target = static_cast<SHADER::CShaderTextureGLES*>(m_shaderTargetTexture.get());
#else
      SHADER::CShaderTextureGLRef sourceTexture(m_shaderSourceWidth, m_shaderSourceHeight,
                                                m_shaderSourceTexture);
      auto* target = static_cast<SHADER::CShaderTextureGL*>(m_shaderTargetTexture.get());
#endif
      const GLint filter =
          m_shaderPreset->GetPasses().front().filterType == SHADER::FilterType::LINEAR ? GL_LINEAR
                                                                                      : GL_NEAREST;
      glBindTexture(m_textureTarget, m_shaderSourceTexture);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);

      // The chain sets the viewport and scissor to the size of what it is
      // writing into and leaves them there, so everything drawn afterwards --
      // the frame itself, and with it the rotation, zoom and pixel ratio that
      // position it -- would be measured against the wrong rectangle. Put them
      // back before drawing anything to the screen.
      // Saved and restored as raw GL state rather than through the render
      // context. The chain sets the viewport with glViewport() directly, so the
      // context's own copy is stale while it runs; putting it back through
      // SetViewPort() would write that stale value into the context and flip it
      // on the way, since the context keeps top-left coordinates and GL keeps
      // bottom-left. Worse, ManageRenderArea() sizes the shader target by
      // reading the context's viewport, so writing to it here feeds the wrong
      // scale back into the next frame -- which is what left CRT presets with a
      // colour cast and no scanlines.
      GLint viewPort[4] = {};
      GLint scissorBox[4] = {};
      glGetIntegerv(GL_VIEWPORT, viewPort);
      glGetIntegerv(GL_SCISSOR_BOX, scissorBox);

      const bool bRendered = m_shaderPreset->RenderUpdate(sourceTexture, *target);

      glViewport(viewPort[0], viewPort[1], viewPort[2], viewPort[3]);
      glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);

      if (bRendered)
      {
        drawTexture = target->GetTextureID();
        bShaded = true;
      }
      else
      {
        CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Video filter failed, drawing the frame unfiltered");
        m_bShadersNeedUpdate = false;
        m_bUseShaderPreset = false;
        DestroyShaderResources();
      }
    }
  }

  // The shader chain wrote a texture that is exactly the destination, already
  // the right way up, so the sub-rect and flip the client's own texture needs
  // do not apply to it.
  if (bShaded)
    rect = CRect(0.0f, 0.0f, 1.0f, 1.0f);

  // Unit 0, because that is where the GUI shader samples from. Binding without
  // selecting it leaves the texture on whichever unit something else last made
  // active, and the shader then reads a unit this renderer never wrote to. Both
  // the software renderers this one is modelled on select it explicitly.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, drawTexture);

  // The vertices below are in screen coordinates, taken from m_rotatedDestCoords,
  // and the GUI shader transforms them with the render context's own matrices.
  // Those matrices are what place and size the picture -- they carry the view
  // mode, zoom, pixel ratio and, for a game rendered into a GUI control, the
  // control's rectangle. Replacing them here with an identity modelview and an
  // Ortho2D spanning the whole viewport discarded all of it, which is why
  // scaling did nothing and the video filter previews drew in the wrong place.
  // Forcing the viewport to (0, 0, x2, y2) compounded it: those are the right
  // and bottom coordinates of the viewport rather than its size, so anything
  // not anchored at the origin was stretched. The renderer this one is modelled
  // on touches none of this state.

  GLint filter = GL_NEAREST;
  if (GetRenderSettings().VideoSettings().GetScalingMethod() == SCALINGMETHOD::LINEAR)
    filter = GL_LINEAR;
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLubyte colour[4];
  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  } vertex[4];

  GLint vertLoc = m_context.GUIShaderGetPos();
  GLint loc = m_context.GUIShaderGetCoord0();
  GLint uniColLoc = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

  // Setup color values
  colour[0] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::R, color);
  colour[1] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::G, color);
  colour[2] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::B, color);
  colour[3] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::A, color);

  for (unsigned int i = 0; i < 4; i++)
  {
    // Setup vertex position values
    vertex[i].x = m_rotatedDestCoords[i].x;
    vertex[i].y = m_rotatedDestCoords[i].y;
    vertex[i].z = 0.0f;
  }

  // Setup texture coordinates
  vertex[0].u1 = vertex[3].u1 = rect.x1;
  vertex[0].v1 = vertex[1].v1 = rect.y1;
  vertex[1].u1 = vertex[2].u1 = rect.x2;
  vertex[2].v1 = vertex[3].v1 = rect.y2;

#if !defined(HAS_GLES)
  // As in DrawBlackBars: a core profile has no default vertex array object, and
  // the draw below is the one that puts the game on the screen. Without this the
  // client's frame arrives complete and is then thrown away by an errored
  // glDrawElements, which is what a desktop GL build showed as a black picture.
  if (m_vao == 0)
    glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);
#endif

  GLuint vertexVBO;
  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex) * 4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(loc);

  GLuint indexVBO;
  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 4, idx, GL_STATIC_DRAW);

  // The GUI shader positions the quad in depth from this. Leaving it unset
  // draws at whatever the uniform happened to hold, which both software
  // renderers avoid by setting it explicitly every frame.
  glUniform1f(depthLoc, -1.0f);

  glUniform4f(uniColLoc, (colour[0] / 255.0f), (colour[1] / 255.0f), (colour[2] / 255.0f),
              (colour[3] / 255.0f));

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#if !defined(HAS_GLES)
  glBindVertexArray(0);
#endif
  glDeleteBuffers(1, &indexVBO);

  m_context.DisableGUIShader();

  // Leave no trace in Kodi's context. The texture sampled here belongs to the
  // game client and is destroyed with the client's context, so leaving it bound
  // hands the GUI a dangling binding to draw with once the game stops.
  glBindTexture(m_textureTarget, 0);

}
