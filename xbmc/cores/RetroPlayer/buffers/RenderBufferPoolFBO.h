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
#pragma once

#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"

#include <mutex>
#include <thread>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
class CRenderContext;

/*!
 * \brief Framebuffers for game clients that render on the GPU themselves
 *
 * The pool owns an OpenGL context shared with the one the window system draws
 * with, and hands the client a framebuffer to render each frame into. Kodi then
 * draws the texture that framebuffer is backed by, so the frame never leaves
 * the GPU.
 *
 * \note Desktop OpenGL only. A client that renders with OpenGL ES is not
 *       supported: this pool is built only where both desktop GL and EGL are
 *       available, and a build without it has no pool that reports
 *       SupportsHardwareRendering(), so such clients are told during
 *       negotiation that hardware rendering is unavailable and can fall back to
 *       software rather than failing later.
 */
class CRenderBufferPoolFBO : public CBaseRenderBufferPool
{
public:
  CRenderBufferPoolFBO(CRenderContext& context);
  ~CRenderBufferPoolFBO() override;

  // implementation of IRenderBufferPool via CRenderBufferPoolSysMem
  bool IsCompatible(const CRenderVideoSettings& renderSettings) const override;

  // implementation of CBaseRenderBufferPool via CRenderBufferPoolSysMem
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;
  bool ConfigureInternal() override;

  bool SupportsHardwareRendering() const override { return true; }
  bool CreateContext(const HwContextProperties& properties) override;
  bool BeginClientFrame() override;
  void EndClientFrame() override;
  void DestroyContext() override;

  /*!
   * \brief Make the client's drawing visible to whoever samples it next
   *
   * Sharing a texture between two contexts does not synchronise access to it.
   * Called on the thread that is about to sample, before it does.
   */
  void WaitForClientFrame();

  /*!
   * \brief Take a copy of the frame the client has just finished
   *
   * The client draws into one framebuffer for the whole session, and clients
   * cache it rather than asking for it again, so it cannot be swapped out from
   * under them. Sampling that framebuffer directly means sampling it while the
   * next frame is being drawn into it, and what is caught missing is whatever
   * the game draws last -- in most titles the HUD and the moving objects.
   *
   * Called on the client's thread, between its frames, where the frame is
   * whole. The copy is what gets published, so the rendering thread never
   * samples a surface the client is drawing into.
   *
   * \return The buffer holding the copy, or nullptr if one could not be taken,
   *         in which case the caller should publish the client's own buffer
   */
  IRenderBuffer* CaptureClientFrame(IRenderBuffer* clientBuffer,
                                    unsigned int width,
                                    unsigned int height) override;

protected:

  // Construction parameters
  CRenderContext& m_context;

  // Configuration parameters
  HwContextProperties m_contextProperties;

  // A context is current on one thread at a time, and the thread a client
  // renders on is whichever one it happens to use. Track who holds it, and what
  // they had before, so it can be handed back.
  unsigned int m_clientFrameDepth{0};

  // Copies of the client's finished frames, used in turn: the rendering thread
  // may still be sampling the one published last frame while this one is being
  // written. Owned by the pool for the life of the stream so they are never
  // handed to the client.
  IRenderBuffer* m_captureBuffers[2]{nullptr, nullptr};
  unsigned int m_captureIndex{0};
  bool m_bLoggedCaptureFailure{false};

  //! \brief Signalled when the client's last frame has been issued
  GLsync m_clientFence{nullptr};
  std::mutex m_fenceMutex;
  std::thread::id m_clientThread;
  EGLDisplay m_prevDisplay{EGL_NO_DISPLAY};
  EGLSurface m_prevDraw{EGL_NO_SURFACE};
  EGLSurface m_prevRead{EGL_NO_SURFACE};
  EGLContext m_prevContext{EGL_NO_CONTEXT};
  EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
  EGLConfig m_eglConfig;
  EGLContext m_eglContext = EGL_NO_CONTEXT;

private:
};
} // namespace RETRO
} // namespace KODI
