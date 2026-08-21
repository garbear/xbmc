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
  // The context is torn down by DestroyContext(), which the stream calls on the
  // game loop thread while the context is still current. By the time the pool
  // is destroyed there is normally nothing left to do, and nothing that can be
  // done: this runs on whichever thread drops the last reference, where the
  // context is not current and deleting its objects would be undefined.
  if (m_eglContext != EGL_NO_CONTEXT)
    CLog::Log(LOGWARNING, "RetroPlayer[RENDER]: FBO context outlived its stream, leaking it");
}

bool CRenderBufferPoolFBO::IsCompatible(const CRenderVideoSettings& renderSettings) const
{
  return CRPRendererFBO::SupportsScalingMethod(renderSettings.GetScalingMethod());
}

bool CRenderBufferPoolFBO::ConfigureInternal()
{
  // This pool only serves hardware-rendered streams, which carry no CPU-side
  // pixel format. Software streams declare a real format and must be left to
  // the DMA and sysmem pools.
  return m_format == AV_PIX_FMT_NONE;
}

IRenderBuffer* CRenderBufferPoolFBO::CreateRenderBuffer(void* header /* = nullptr */)
{
  if (m_eglContext == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: No shared context; the stream must create one first");
    return nullptr;
  }

  // Framebuffer objects are not shared between contexts, so a buffer built
  // outside the client's context would be useless to it. This also keeps the
  // rendering thread from quietly allocating one in Kodi's context.
  if (m_clientFrameDepth == 0)
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Refusing to build a buffer outside the client's context");
    return nullptr;
  }

  return new CRenderBufferFBO(m_context, m_contextProperties.depth,
                              m_contextProperties.stencil,
                              m_contextProperties.bottomLeftOrigin);
}

bool CRenderBufferPoolFBO::CreateContext(const HwContextProperties& properties)
{
  // Idempotent, so reopening a stream on the same pool is harmless
  if (m_eglContext != EGL_NO_CONTEXT)
    return true;

  // Remembered so buffers can be built with the attachments the client wants
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

  // Honour the context the core asked for. A core written against legacy
  // OpenGL will not run in a core profile - its shaders and fixed-function
  // calls are simply absent there - so requesting the wrong profile leaves the
  // core unable to build its resources, with no obvious symptom beyond a core
  // that never draws.
  // The version asked for is a minimum, not an exact match. A client asking for
  // OpenGL ES 3 means "3.0 or better", the same as it does everywhere else in
  // libretro, and pinning it to exactly 3.0 costs it everything added since:
  // compute shaders and shader storage buffers arrived in 3.1, and a core that
  // uses them resolves those entry points to NULL on a 3.0 context and calls
  // them anyway. Try the later versions first and fall back to what was asked.
  //
  // ES only. Its minor versions are additive, so a higher one can only offer
  // more, whereas desktop GL versions interact with the profile requested
  // below and are left exactly as the client asked for them.
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

  const std::string apiName = properties.embedded ? "OpenGL ES"
                              : properties.coreProfile ? "OpenGL core profile"
                                                       : "OpenGL compatibility profile";

  // This is the version and profile check: rather than testing the request
  // against a hardcoded table, ask the driver for it and let it refuse. A
  // refusal fails the stream cleanly, while the client can still fall back to
  // software rendering.
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

    // Describes what was asked for, so a rejection says which request the
    // driver could not meet
    contextName = apiName;
    if (major != 0)
      contextName += StringUtils::Format(" {}.{}", major, minor);

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, winSystem->GetEGLContext(),
                                    contextAttribs.data());
    if (m_eglContext != EGL_NO_CONTEXT)
    {
      // "Sharing Kodi's objects" rather than "shared", because libretro uses
      // that word for something else: a client asking, through
      // RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT, to create further contexts of
      // its own. Kodi does not offer that, and a client that wanted it says so
      // in the log right next to this line.
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

  // Deliberately not made current here. This runs on whichever thread opened
  // the stream, which for some clients is Kodi's own rendering thread, and a
  // context binding is per-thread: taking that thread over would cost Kodi the
  // window surface it presents with. BeginClientFrame() binds it around the
  // client's work instead, on the thread doing that work.
  return true;
}

bool CRenderBufferPoolFBO::BeginClientFrame()
{
  if (m_eglContext == EGL_NO_CONTEXT)
    return false;

  const std::thread::id thisThread = std::this_thread::get_id();

  if (m_clientFrameDepth > 0)
  {
    // Nested, which is normal: allocating buffers binds the context too, and
    // that happens inside the client's frame for some clients.
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
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to make the client's context current, EGL "
                        "error {:#x}", eglGetError());
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

  // The mirror of the check BeginClientFrame makes. That refuses to take the
  // context on a thread while another thread holds it, and returns false
  // without nesting -- but callers pair a Begin with an End regardless of what
  // Begin answered, so the refused call used to arrive here and decrement all
  // the same. The depth then reached zero a level early and the context was
  // released out from under the thread that really held it.
  //
  // On Kodi's rendering thread that is expensive: releasing the client's
  // surfaceless context there leaves the thread with no default framebuffer, so
  // the next clear has nothing to write to and the driver rejects it. The
  // stream opens across two threads, which is where the mismatch comes from,
  // and the picture recovers on the following frame -- so it shows up as a
  // single incomplete-framebuffer error just after a game starts.
  if (m_clientThread != std::this_thread::get_id())
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: A thread that does not hold the client's context "
                        "tried to end its frame; ignoring");
    return;
  }

  if (--m_clientFrameDepth > 0)
    return;

  // Fence the client's drawing before letting go of its context.
  //
  // The client renders into the framebuffer on its own thread with its own
  // context, and Kodi samples the resulting texture from the rendering thread.
  // Sharing an object between two contexts does not synchronise access to it:
  // without a fence, Kodi is free to sample a texture whose writes have not
  // landed, and it will draw whatever was there. What goes missing is the last
  // thing the client drew, which in most games is the HUD.
  //
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

  // Taken once and kept. Asking the pool each frame would hand the client's own
  // buffer back out again as soon as the rendering thread released it.
  if (target == nullptr)
  {
    target = GetBuffer(width, height);

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

  glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(srcFramebuffer));
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(dstFramebuffer));

  // Straight copy: the frame is oriented and sized as the client left it, and
  // the renderer applies the same conventions to the copy as to the original.
  glBlitFramebuffer(0, 0, static_cast<GLint>(width), static_cast<GLint>(height), 0, 0,
                    static_cast<GLint>(width), static_cast<GLint>(height), GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

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

  // Buffers own framebuffers and textures belonging to this context, so they
  // have to go while it is still current.
  Flush();

  if (bBound)
    EndClientFrame();

  eglDestroyContext(m_eglDisplay, m_eglContext);

  m_eglContext = EGL_NO_CONTEXT;
  m_eglDisplay = EGL_NO_DISPLAY;
}
