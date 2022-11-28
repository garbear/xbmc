/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPBaseRenderer.h"
#include "cores/GameSettings.h"
#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

#include <atomic>
#include <stdint.h>
#include <vector>

#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
class CRendererFactoryFBO : public IRendererFactory
{
public:
  ~CRendererFactoryFBO() override = default;

  // implementation of IRendererFactory
  std::string RenderSystemName() const override;
  CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                  CRenderContext& context,
                                  std::shared_ptr<IRenderBufferPool> bufferPool) override;
  RenderBufferPoolVector CreateBufferPools(CRenderContext& context) override;
};

class CRPRendererFBO : public CRPBaseRenderer
{
public:
  CRPRendererFBO(const CRenderSettings& renderSettings,
                 CRenderContext& context,
                 std::shared_ptr<IRenderBufferPool> bufferPool);
  ~CRPRendererFBO() override;

  // implementation of CRPBaseRenderer
  bool Supports(RENDERFEATURE feature) const override;
  SCALINGMETHOD GetDefaultScalingMethod() const override { return SCALINGMETHOD::NEAREST; }

  static bool SupportsScalingMethod(SCALINGMETHOD method);

protected:
  // implementation of CRPBaseRenderer
  void RenderInternal(bool clear, uint8_t alpha) override;
  void FlushInternal() override;

  /*!
   * \brief Set the entire backbuffer to black
   */
  void ClearBackBuffer();

  /*!
   * \brief Draw black bars around the video quad
   *
   * This is more efficient than glClear() since it only sets pixels to
   * black that aren't going to be overwritten by the game.
   */
  void DrawBlackBars();

  virtual void Render(uint8_t alpha);

  GLenum m_textureTarget = GL_TEXTURE_2D;
  float m_clearColour = 0.0f;
};
} // namespace RETRO
} // namespace KODI
