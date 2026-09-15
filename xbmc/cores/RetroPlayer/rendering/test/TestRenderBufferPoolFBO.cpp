/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererFBO.h"

#ifdef HAS_RP_RENDERER_FBO

#include "cores/RetroPlayer/buffers/RenderBufferPoolFBO.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"

#include <memory>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

namespace
{
class CTestCaptureBuffer : public CRenderBufferSysMem
{
public:
  bool Allocate(AVPixelFormat, unsigned int width, unsigned int height) override
  {
    SetSize(width, height);
    return true;
  }
  bool UploadTexture() override { return true; }
};

class CTestCapturePool : public CRenderBufferPoolFBO
{
public:
  using CRenderBufferPoolFBO::CRenderBufferPoolFBO;
  using CRenderBufferPoolFBO::GetCaptureBuffer;

  void Return(IRenderBuffer* buffer) override { CBaseRenderBufferPool::Return(buffer); }

protected:
  IRenderBuffer* CreateRenderBuffer(void*) override { return new CTestCaptureBuffer; }
};
} // namespace

TEST(TestRenderBufferPoolFBO, SameSizeCaptureResumesAfterFlush)
{
  CPlaybackTestEnvironment environment;
  auto pool = std::make_shared<CTestCapturePool>(environment.ProcessInfo().GetRenderContext());
  auto* first = pool->GetCaptureBuffer(320, 240);
  ASSERT_NE(first, nullptr);
  first->Release();

  pool->Flush();
  ASSERT_FALSE(pool->IsConfigured());

  auto* second = pool->GetCaptureBuffer(320, 240);
  ASSERT_NE(second, nullptr);
  EXPECT_TRUE(pool->IsConfigured());
  EXPECT_EQ(second->GetWidth(), 320);
  EXPECT_EQ(second->GetHeight(), 240);
  second->Release();
}

TEST(TestRenderBufferPoolFBO, CaptureDimensionsCanChangeAfterFlush)
{
  CPlaybackTestEnvironment environment;
  auto pool = std::make_shared<CTestCapturePool>(environment.ProcessInfo().GetRenderContext());
  auto* first = pool->GetCaptureBuffer(320, 240);
  ASSERT_NE(first, nullptr);
  first->Release();
  pool->Flush();

  auto* second = pool->GetCaptureBuffer(640, 480);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(second->GetWidth(), 640);
  EXPECT_EQ(second->GetHeight(), 480);
  second->Release();
}

namespace
{
class CEGLCapturePool : public CRenderBufferPoolFBO
{
public:
  CEGLCapturePool(CRenderContext& context, EGLDisplay display, EGLContext eglContext)
    : CRenderBufferPoolFBO(context)
  {
    m_eglDisplay = display;
    m_eglContext = eglContext;
  }
};
} // namespace

class TestRenderBufferPoolFBOWithContext : public testing::Test
{
protected:
  void SetUp() override
  {
    if (eglGetCurrentContext() != EGL_NO_CONTEXT)
      GTEST_SKIP() << "An EGL context is already current";

    m_previousAPI = eglQueryAPI();
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY || !eglInitialize(m_display, nullptr, nullptr))
      GTEST_SKIP() << "No EGL display is available";
    m_initialized = true;

#if defined(HAS_GLES)
    const EGLenum api = EGL_OPENGL_ES_API;
    const EGLint renderable = EGL_OPENGL_ES3_BIT_KHR;
    const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
#else
    const EGLenum api = EGL_OPENGL_API;
    const EGLint renderable = EGL_OPENGL_BIT;
    const EGLint contextAttributes[] = {EGL_CONTEXT_MAJOR_VERSION_KHR,
                                        3,
                                        EGL_CONTEXT_MINOR_VERSION_KHR,
                                        2,
                                        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
                                        EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
                                        EGL_NONE};
#endif
    if (!eglBindAPI(api))
      GTEST_SKIP() << "The required EGL client API is unavailable";
    const EGLint configAttributes[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE,
                                       renderable, EGL_NONE};
    EGLConfig config{};
    EGLint count = 0;
    if (!eglChooseConfig(m_display, configAttributes, &config, 1, &count) || count == 0)
      GTEST_SKIP() << "No compatible EGL configuration is available";
    const EGLContext context =
        eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttributes);
    if (context == EGL_NO_CONTEXT)
      GTEST_SKIP() << "The required GL context is unavailable";

    m_pool = std::make_shared<CEGLCapturePool>(m_environment.ProcessInfo().GetRenderContext(),
                                               m_display, context);
    m_current = m_pool->BeginClientFrame();
    if (!m_current)
      GTEST_SKIP() << "Surfaceless EGL contexts are unavailable";
    ASSERT_TRUE(m_pool->Configure(AV_PIX_FMT_NONE));
  }

  void TearDown() override
  {
    if (m_current)
      m_pool->EndClientFrame();
    m_pool.reset();
    if (m_initialized)
      eglTerminate(m_display);
    if (m_previousAPI != EGL_NONE)
      eglBindAPI(m_previousAPI);
  }

  CPlaybackTestEnvironment m_environment;
  std::shared_ptr<CEGLCapturePool> m_pool;
  EGLDisplay m_display{EGL_NO_DISPLAY};
  EGLenum m_previousAPI{EGL_NONE};
  bool m_initialized{false};
  bool m_current{false};
};

TEST_F(TestRenderBufferPoolFBOWithContext, CaptureDiscardsAlphaBeforeShaderCopy)
{
  auto* client = static_cast<CRenderBufferFBO*>(m_pool->GetBuffer(4, 4));
  ASSERT_NE(client, nullptr);
  const auto clientFramebuffer = client->GetCurrentFramebuffer();
  glBindFramebuffer(GL_FRAMEBUFFER, clientFramebuffer);
  glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  auto* captured = static_cast<CRenderBufferFBO*>(m_pool->CaptureClientFrame(client, 4, 4));
  EXPECT_NE(captured, nullptr);
  if (captured)
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, captured->GetCurrentFramebuffer());
    unsigned char pixel[4]{};
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    EXPECT_EQ(pixel[0], 255);
    EXPECT_EQ(pixel[1], 0);
    EXPECT_EQ(pixel[2], 0);
    EXPECT_EQ(pixel[3], 255);

    auto* shaderCopy = static_cast<CRenderBufferFBO*>(m_pool->GetBuffer(4, 4));
    EXPECT_NE(shaderCopy, nullptr);
    if (shaderCopy)
    {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shaderCopy->GetCurrentFramebuffer());
      glBlitFramebuffer(0, 4, 4, 0, 0, 0, 4, 4, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, shaderCopy->GetCurrentFramebuffer());
      glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
      EXPECT_EQ(pixel[0], 255);
      EXPECT_EQ(pixel[3], 255);
      shaderCopy->Release();
    }
    captured->Release();
  }

  glBindFramebuffer(GL_READ_FRAMEBUFFER, clientFramebuffer);
  unsigned char clientPixel[4]{};
  glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, clientPixel);
  EXPECT_EQ(clientPixel[0], 255);
  EXPECT_EQ(clientPixel[3], 0);
  EXPECT_EQ(client->GetCurrentFramebuffer(), clientFramebuffer);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
  client->Release();
}

#endif
