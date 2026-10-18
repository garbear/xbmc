/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPBaseRenderer.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

#include <map>
#include <memory>
#include <stdint.h>

#include "system_gl.h"

namespace KODI
{
namespace SHADER
{
class CShaderTextureGLES;
class CShaderTextureGLESRef;
} // namespace SHADER

namespace RETRO
{
class CRenderBufferOpenGLES;
class CRPHdrShaderGLES;

class CRendererFactoryOpenGLES : public IRendererFactory
{
public:
  ~CRendererFactoryOpenGLES() override = default;

  // Implementation of IRendererFactory
  std::string RenderSystemName() const override;
  CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                  CRenderContext& context,
                                  std::shared_ptr<IRenderBufferPool> bufferPool) override;
  RenderBufferPoolVector CreateBufferPools(CRenderContext& context) override;
};

class CRPRendererOpenGLES : public CRPBaseRenderer
{
public:
  CRPRendererOpenGLES(const CRenderSettings& renderSettings,
                      CRenderContext& context,
                      std::shared_ptr<IRenderBufferPool> bufferPool);
  ~CRPRendererOpenGLES() override;

  // Implementation of CRPBaseRenderer
  bool Supports(RENDERFEATURE feature) const override;
  SCALINGMETHOD GetDefaultScalingMethod() const override { return SCALINGMETHOD::NEAREST; }

  static bool SupportsScalingMethod(SCALINGMETHOD method);
  bool SupportsHdr() const override { return true; }

protected:
  struct RenderBufferTextures
  {
    std::shared_ptr<SHADER::CShaderTextureGLESRef> sourceTexture;
    std::shared_ptr<SHADER::CShaderTextureGLES> targetTexture;
    std::shared_ptr<SHADER::CShaderTextureGLES> hdrTargetTexture;
  };

  /*!
   * \brief The texture a shader preset's last pass renders into
   */
  SHADER::CShaderTextureGLES& GetTargetTexture(RenderBufferTextures& textures, const CSize& size);

  /*!
   * \brief Draw the texture bound to unit 0 into the destination, for SDR or HDR
   *
   * \param alpha The frame's opacity
   * \param width, height Size of the texture, which the source rectangle is in
   */
  void DrawFrame(uint8_t alpha, float width, float height);

  // Implementation of CRPBaseRenderer
  void RenderInternal(uint8_t alpha) override;
  void FlushInternal() override;

  virtual void Render(uint8_t alpha);

  std::map<CRenderBufferOpenGLES*, std::unique_ptr<RenderBufferTextures>> m_RBTexturesMap;

  const GLenum m_textureTarget = GL_TEXTURE_2D;

private:
  bool LoadHdrShader();

  std::unique_ptr<CRPHdrShaderGLES> m_hdrShader;
  bool m_hdrShaderFailed{false};
};
} // namespace RETRO
} // namespace KODI
