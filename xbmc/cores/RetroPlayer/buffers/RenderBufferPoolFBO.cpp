/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RenderBufferPoolFBO.h"

#include "RenderBufferFBO.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererFBO.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/linux/WinSystemEGL.h"

#include <string>
#include <utility>
#include <vector>

using namespace KODI;
using namespace RETRO;

CRenderBufferPoolFBO::CRenderBufferPoolFBO(CRenderContext& context) : m_context(context)
{
}

CRenderBufferPoolFBO::~CRenderBufferPoolFBO()
{
  // Nothing can be deleted here: this runs on whichever thread drops the last
  // reference, where the context is not current.
  if (m_eglContext != EGL_NO_CONTEXT)
    CLog::Log(LOGWARNING, "RetroPlayer[RENDER]: FBO context outlived its stream, leaking it");
}

bool CRenderBufferPoolFBO::SupportsHardwareRendering() const
{
  // Asked of the window system rather than the build: a build carrying this
  // pool can still be running where there is no EGL display to share.
  auto* winSystem =
      dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(CServiceBroker::GetWinSystem());
  if (winSystem == nullptr)
    return false;

  return winSystem->GetEGLDisplay() != EGL_NO_DISPLAY;
}

bool CRenderBufferPoolFBO::IsCompatible(const CRenderVideoSettings& renderSettings) const
{
  return CRPRendererFBO::SupportsScalingMethod(renderSettings.GetScalingMethod());
}

bool CRenderBufferPoolFBO::ConfigureInternal()
{
  // Hardware-rendered streams carry no CPU-side pixel format. Software ones
  // declare a real one and belong to the DMA and sysmem pools.
  return m_format == AV_PIX_FMT_NONE;
}

IRenderBuffer* CRenderBufferPoolFBO::CreateRenderBuffer(void* header /* = nullptr */)
{
  if (m_eglContext == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: No shared context; the stream must create one first");
    return nullptr;
  }

  // Framebuffer objects are not shared between contexts, so one built outside
  // the client's context would be useless to it.
  if (m_clientFrameDepth == 0)
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Refusing to build a buffer outside the client's context");
    return nullptr;
  }

  return new CRenderBufferFBO(m_context, m_contextProperties.depth, m_contextProperties.stencil,
                              m_contextProperties.bottomLeftOrigin);
}

bool CRenderBufferPoolFBO::CreateContext(const HwContextProperties& properties)
{
  // Idempotent, so reopening a stream on the same pool is harmless
  if (m_eglContext != EGL_NO_CONTEXT)
    return true;

  m_contextProperties = properties;

  auto winSystem =
      dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(CServiceBroker::GetWinSystem());
  if (winSystem == nullptr)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Window system does not use EGL");
    return false;
  }

  m_eglDisplay = winSystem->GetEGLDisplay();

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CLog::Log(LOGERROR, "failed to get EGL display");
    return false;
  }

  if (!eglInitialize(m_eglDisplay, nullptr, nullptr))
  {
    CLog::Log(LOGERROR, "failed to initialize EGL display");
    return false;
  }

  // clang-format off

  // Ask EGL for whichever API this build renders with. The pool, the buffers
  // and the renderer are common to both; only the context differs.
#if defined(HAS_GLES)
  eglBindAPI(EGL_OPENGL_ES_API);
#else
  eglBindAPI(EGL_OPENGL_API);
#endif

  EGLint attribs[] =
  {
#if defined(HAS_GLES)
    // ES3 rather than ES2: the framebuffer objects, the depth and stencil
    // attachments and the sampling this pool relies on are all core in ES3.
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
#else
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
    // The context is only ever made current without a surface, so the surface
    // type is a formality -- but eglChooseConfig defaults it to EGL_WINDOW_BIT
    // and matches on it either way, so it has to name something the platform
    // really offers. Window is the one every platform Kodi runs on provides.
    // Pbuffer is not: on GBM, configs come from the GBM formats and advertise
    // window only, so asking for a pbuffer matches nothing at all and every
    // hardware core fails to get a context.
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_ALPHA_SIZE, 8,
    EGL_NONE
  };

  EGLint neglconfigs;
  if (!eglChooseConfig(m_eglDisplay, attribs, &m_eglConfig, 1, &neglconfigs))
  {
    CLog::Log(LOGERROR, "Failed to query number of EGL configs");
    return false;
  }

  if (neglconfigs <= 0)
  {
    CLog::Log(LOGERROR, "No suitable EGL configs found");
    return false;
  }

  // clang-format on

  // In libretro the version a client asks for is a minimum, so try later ones
  // first and fall back to the request. ES only: its minor versions are purely
  // additive, whereas a desktop GL version interacts with the profile below.
  std::vector<std::pair<unsigned int, unsigned int>> versions;
  if (properties.versionMajor != 0)
  {
    if (properties.embedded && properties.versionMajor == 3)
    {
      for (unsigned int minor = 2; minor > properties.versionMinor; --minor)
        versions.emplace_back(3, minor);
    }
    versions.emplace_back(properties.versionMajor, properties.versionMinor);
  }
  else
  {
    // Nothing asked for, so let the driver decide
    versions.emplace_back(0, 0);
  }

  const std::string apiName = properties.embedded      ? "OpenGL ES"
                              : properties.coreProfile ? "OpenGL core profile"
                                                       : "OpenGL compatibility profile";

  // Ask the driver for each in turn rather than keeping a table of what it
  // supports. A refusal fails the stream cleanly and the client falls back.
  std::string contextName;
  for (const auto& [major, minor] : versions)
  {
    std::vector<EGLint> contextAttribs;

    if (major != 0)
    {
      contextAttribs.push_back(EGL_CONTEXT_MAJOR_VERSION_KHR);
      contextAttribs.push_back(static_cast<EGLint>(major));
      contextAttribs.push_back(EGL_CONTEXT_MINOR_VERSION_KHR);
      contextAttribs.push_back(static_cast<EGLint>(minor));
    }

    if (!properties.embedded)
    {
      contextAttribs.push_back(EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
      contextAttribs.push_back(properties.coreProfile
                                   ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
                                   : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR);
    }

    contextAttribs.push_back(EGL_NONE);

    contextName = apiName;
    if (major != 0)
      contextName += StringUtils::Format(" {}.{}", major, minor);

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, winSystem->GetEGLContext(),
                                    contextAttribs.data());
    if (m_eglContext != EGL_NO_CONTEXT)
    {
      CLog::Log(LOGINFO,
                "RetroPlayer[RENDER]: Created a {} context for the game client, sharing Kodi's "
                "objects",
                contextName);
      break;
    }
  }

  if (m_eglContext == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Game client asked for a {} context, which this system cannot "
              "provide (EGL error {:#x})",
              contextName, eglGetError());
    return false;
  }

  // Not made current here: a binding is per-thread, and the thread that opened
  // the stream can be Kodi's own rendering thread, which would lose the window
  // surface it presents with. BeginClientFrame() binds it around the work.
  return true;
}

bool CRenderBufferPoolFBO::BeginClientFrame()
{
  // A client negotiates hardware rendering before it opens the stream that
  // creates the context, so having nothing to bind yet is not a failure.
  if (m_eglContext == EGL_NO_CONTEXT)
    return true;

  const std::thread::id thisThread = std::this_thread::get_id();

  if (m_clientFrameDepth > 0)
  {
    // Nested because allocating a buffer binds the context too, which some
    // clients do inside their frame.
    if (m_clientThread != thisThread)
    {
      CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Client context is in use by another thread");
      return false;
    }

    ++m_clientFrameDepth;
    return true;
  }

  m_prevDisplay = eglGetCurrentDisplay();
  m_prevDraw = eglGetCurrentSurface(EGL_DRAW);
  m_prevRead = eglGetCurrentSurface(EGL_READ);
  m_prevContext = eglGetCurrentContext();

  if (!eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext))
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Failed to make the client's context current, EGL "
              "error {:#x}",
              eglGetError());
    return false;
  }

  m_clientThread = thisThread;
  m_clientFrameDepth = 1;

  return true;
}

void CRenderBufferPoolFBO::EndClientFrame()
{
  if (m_clientFrameDepth == 0)
    return;

  // The mirror of the check BeginClientFrame makes: a Begin refused because
  // another thread holds the context does not nest, so the End its caller pairs
  // with it must not unnest. Decrementing regardless would reach zero a level
  // early and release the context from under the thread that holds it.
  if (m_clientThread != std::this_thread::get_id())
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: A thread that does not hold the client's context "
                        "tried to end its frame; ignoring");
    return;
  }

  if (--m_clientFrameDepth > 0)
    return;

  // Sharing an object between two contexts does not synchronise access to it,
  // so without this Kodi can sample a texture whose writes have not landed.
  // A fence rather than glFinish, so the ordering is imposed on the GPU and the
  // game loop is not stalled waiting for it to drain.
  {
    std::unique_lock<std::mutex> lock{m_fenceMutex};

    if (m_clientFence != nullptr)
      glDeleteSync(m_clientFence);

    m_clientFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // The fence is only guaranteed to be reachable from another context once
    // the commands before it have been flushed to the driver
    glFlush();
  }

  // Give the thread back exactly what it had, so Kodi keeps its surface if this
  // happened to be its rendering thread
  if (m_prevContext != EGL_NO_CONTEXT && m_prevDisplay != EGL_NO_DISPLAY)
    eglMakeCurrent(m_prevDisplay, m_prevDraw, m_prevRead, m_prevContext);
  else
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  m_clientThread = std::thread::id();
  m_prevDisplay = EGL_NO_DISPLAY;
  m_prevDraw = EGL_NO_SURFACE;
  m_prevRead = EGL_NO_SURFACE;
  m_prevContext = EGL_NO_CONTEXT;
}

void CRenderBufferPoolFBO::WaitForClientFrame()
{
  std::unique_lock<std::mutex> lock{m_fenceMutex};

  if (m_clientFence == nullptr)
    return;

  // Order this context's reads after the client's writes. The wait is on the
  // GPU, so this returns immediately and costs the caller nothing.
  glWaitSync(m_clientFence, 0, GL_TIMEOUT_IGNORED);
}

IRenderBuffer* CRenderBufferPoolFBO::CaptureClientFrame(IRenderBuffer* clientBuffer,
                                                        unsigned int width,
                                                        unsigned int height)
{
  if (clientBuffer == nullptr || width == 0 || height == 0)
    return nullptr;

  const uintptr_t srcFramebuffer = clientBuffer->GetCurrentFramebuffer();
  if (srcFramebuffer == 0)
    return nullptr;

  IRenderBuffer*& target = m_captureBuffers[m_captureIndex];

  const CRenderBufferFBO* clientFbo = static_cast<const CRenderBufferFBO*>(clientBuffer);

  // A capture buffer's texture size is fixed when it is allocated, and the
  // renderer measures its sampling coordinates against that size. One kept after
  // the client's framebuffer has grown is sampled past its own edge.
  if (target != nullptr)
  {
    const CRenderBufferFBO* targetFbo = static_cast<const CRenderBufferFBO*>(target);
    if (targetFbo->TextureWidth() < clientFbo->TextureWidth() ||
        targetFbo->TextureHeight() < clientFbo->TextureHeight())
    {
      target->Release();
      target = nullptr;
    }
  }

  // Taken once and kept. Asking the pool each frame would hand the client's own
  // buffer back out again as soon as the rendering thread released it.
  if (target == nullptr)
  {
    // Sized to the client's framebuffer, not to this frame: a client is free to
    // change resolution between frames, and every frame it draws has to fit.
    target = GetBuffer(clientFbo->TextureWidth(), clientFbo->TextureHeight());

    if (target != nullptr && target->GetCurrentFramebuffer() == 0)
    {
      target->Release();
      target = nullptr;
    }

    if (target == nullptr)
    {
      if (!m_bLoggedCaptureFailure)
      {
        CLog::Log(LOGWARNING, "RetroPlayer[RENDER]: No buffer to copy the client's frame into, the "
                              "rendering thread will sample the one being drawn into");
        m_bLoggedCaptureFailure = true;
      }
      return nullptr;
    }
  }

  const uintptr_t dstFramebuffer = target->GetCurrentFramebuffer();

  // Kodi may itself be rendering through a framebuffer, so what was bound is
  // put back rather than assuming the default was current
  GLint prevRead = 0;
  GLint prevDraw = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevRead);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDraw);

  // A blit is scissored like any other draw, and the test belongs to the client
  // whose context this is. One left enabled over part of the screen would copy
  // that part and leave the rest of the frame as it was.
  const GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
  if (scissorEnabled)
    glDisable(GL_SCISSOR_TEST);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(srcFramebuffer));
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(dstFramebuffer));

  glBlitFramebuffer(0, 0, static_cast<GLint>(width), static_cast<GLint>(height), 0, 0,
                    static_cast<GLint>(width), static_cast<GLint>(height), GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevRead));
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(prevDraw));

  if (scissorEnabled)
    glEnable(GL_SCISSOR_TEST);

  m_captureIndex = (m_captureIndex + 1) % 2;

  return target;
}

void CRenderBufferPoolFBO::DestroyContext()
{
  // DestroyContext() is broadcast to every pool, including those that never
  // created a context, so there is usually nothing to do here.
  if (m_eglContext == EGL_NO_CONTEXT)
    return;

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Destroying shared FBO context");

  // Deleting this context's objects needs it current
  const bool bBound = BeginClientFrame();

  // Held for the life of the stream, so they have to be given back before the
  // pool is flushed or their framebuffers outlive the context that owns them.
  for (IRenderBuffer*& captureBuffer : m_captureBuffers)
  {
    if (captureBuffer != nullptr)
    {
      captureBuffer->Release();
      captureBuffer = nullptr;
    }
  }
  m_captureIndex = 0;

  Flush();

  if (bBound)
    EndClientFrame();

  eglDestroyContext(m_eglDisplay, m_eglContext);

  m_eglContext = EGL_NO_CONTEXT;
  m_eglDisplay = EGL_NO_DISPLAY;
}
