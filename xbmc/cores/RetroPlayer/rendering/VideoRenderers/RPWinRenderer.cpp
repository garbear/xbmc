/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPWinRenderer.h"

#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderTranslator.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "cores/RetroPlayer/shaders/windows/RPWinOutputShader.h"
#include "cores/RetroPlayer/shaders/windows/ShaderPresetDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTextureDX.h"
#include "guilib/D3DResource.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/log.h"

extern "C"
{
#include <libswscale/swscale.h>
}

#include <cstring>

using namespace KODI;
using namespace RETRO;

// --- CWinRendererFactory -----------------------------------------------------

std::string CWinRendererFactory::RenderSystemName() const
{
  return "DirectX";
}

CRPBaseRenderer* CWinRendererFactory::CreateRenderer(const CRenderSettings& settings,
                                                     CRenderContext& context,
                                                     std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPWinRenderer(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CWinRendererFactory::CreateBufferPools(CRenderContext& context)
{
  return {std::make_shared<CWinRenderBufferPool>()};
}

// --- CWinRenderBuffer --------------------------------------------------------

CWinRenderBuffer::CWinRenderBuffer(AVPixelFormat pixFormat, DXGI_FORMAT dxFormat)
  : m_pixFormat(pixFormat), m_targetDxFormat(dxFormat), m_targetPixFormat(GetPixFormat(dxFormat))
{
}

bool CWinRenderBuffer::CreateTexture()
{
  if (!m_intermediateTarget->GetPointer()->Create(m_width, m_height, 1, D3D11_USAGE_DYNAMIC,
                                                  m_targetDxFormat))
  {
    CLog::Log(LOGERROR, "WinRenderer: Intermediate render target creation failed");
    return false;
  }

  return true;
}

bool CWinRenderBuffer::GetTexture(uint8_t*& data, unsigned int& stride)
{
  // Scale and upload texture
  D3D11_MAPPED_SUBRESOURCE destlr;
  if (!m_intermediateTarget->GetPointer()->LockRect(0, &destlr, D3D11_MAP_WRITE_DISCARD))
  {
    CLog::Log(LOGERROR, "WinRenderer: Failed to lock swtarget texture into memory");
    return false;
  }

  data = static_cast<uint8_t*>(destlr.pData);
  stride = destlr.RowPitch;

  return true;
}

bool CWinRenderBuffer::ReleaseTexture()
{
  if (!m_intermediateTarget->GetPointer()->UnlockRect(0))
  {
    CLog::Log(LOGERROR, "WinRenderer: Failed to unlock swtarget texture");
    return false;
  }

  return true;
}

bool CWinRenderBuffer::UploadTexture()
{
  if (m_targetDxFormat == DXGI_FORMAT_UNKNOWN)
  {
    CLog::Log(LOGERROR, "WinRenderer: Invalid DX texture format");
    return false;
  }

  if (!CreateScalingContext())
    return false;

  // Create intermediate texture
  if (!m_intermediateTarget)
  {
    m_intermediateTarget.reset(new SHADER::CShaderTextureCD3D(new CD3DTexture));
    if (!CreateTexture())
    {
      m_intermediateTarget.reset();
      return false;
    }
  }

  uint8_t* destData = nullptr;
  unsigned int destStride = 0;
  if (!GetTexture(destData, destStride))
    return false;

  const unsigned int sourceStride = static_cast<unsigned int>(m_data.size() / m_height);
  ScalePixels(m_data.data(), sourceStride, destData, destStride);

  if (!ReleaseTexture())
    return false;

  return true;
}

bool CWinRenderBuffer::CreateScalingContext()
{
  if (m_swsContext == nullptr)
  {
    m_swsContext = sws_getContext(m_width, m_height, m_pixFormat, m_width, m_height,
                                  m_targetPixFormat, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    if (m_swsContext == nullptr)
    {
      CLog::Log(LOGERROR, "WinRenderer: Failed to create swscale context");
      return false;
    }
  }

  return true;
}

void CWinRenderBuffer::ScalePixels(const uint8_t* source,
                                   unsigned int sourceStride,
                                   uint8_t* target,
                                   unsigned int targetStride)
{

  // libav doesn't provide a pixel format for R10G10B10A2, so we have to do
  // it ourselves.
  if (m_targetDxFormat == DXGI_FORMAT_R10G10B10A2_UNORM)
  {
    ScalePixelsR10G10B10A2(source, sourceStride, target, targetStride);
  }
  else
  {
    uint8_t* src[] = {const_cast<uint8_t*>(source), nullptr, nullptr, nullptr};
    int srcStride[] = {static_cast<int>(sourceStride), 0, 0, 0};
    uint8_t* dst[] = {target, nullptr, nullptr, nullptr};
    int dstStride[] = {static_cast<int>(targetStride), 0, 0, 0};

    sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);
  }
}

void CWinRenderBuffer::ScalePixelsR10G10B10A2(const uint8_t* source,
                                              unsigned int sourceStride,
                                              uint8_t* target,
                                              unsigned int targetStride)
{
  // Set the alpha bits to fully opaque
  const uint64_t alpha = 0x1;

  switch (m_pixFormat)
  {
    case AV_PIX_FMT_0RGB32: // 0RGB32 is XRGBXRGB
    {
      for (unsigned int y = 0; y < m_height; ++y)
      {
        for (unsigned int x = 0; x < m_width; ++x)
        {
          const uint32_t sourcePixel =
              reinterpret_cast<const uint32_t*>(source + y * sourceStride)[x];
          uint32_t& targetPixel = reinterpret_cast<uint32_t*>(target + y * targetStride)[x];

          // Get the 8-bit R value from 0RGB32
          uint64_t r = (sourcePixel & 0x00ff0000) >> 16;

          // Get the 8-bit G value from 0RGB32
          uint64_t g = (sourcePixel & 0x0000ff00) >> 8;

          // Get the 8-bit B value from 0RGB32
          uint64_t b = (sourcePixel & 0x000000ff);

          // Scale the R value from 8-bit to 10-bit
          r = r * (1ULL << 10) / (1ULL << 8);

          // Scale the G value from 8-bit to 10-bit
          g = g * (1ULL << 10) / (1ULL << 8);

          // Scale the B value from 8-bit to 10-bit
          b = b * (1ULL << 10) / (1ULL << 8);

          // Combine the RGBA bits into R10G10B10A2
          targetPixel = (r << 0) | (g << 10) | (b << 20) | (alpha << 30);
        }
      }
      break;
    }
    case AV_PIX_FMT_RGBA: // RGBA is RGBARGBA
    {
      for (unsigned int y = 0; y < m_height; ++y)
      {
        for (unsigned int x = 0; x < m_width; ++x)
        {
          const uint32_t sourcePixel =
              reinterpret_cast<const uint32_t*>(source + y * sourceStride)[x];
          uint32_t& targetPixel = reinterpret_cast<uint32_t*>(target + y * targetStride)[x];

          // Get the 8-bit R value from RGBA
          uint64_t r = (sourcePixel & 0xff000000) >> 24;

          // Get the 8-bit G value from RGBA
          uint64_t g = (sourcePixel & 0x00ff0000) >> 16;

          // Get the 8-bit B value from RGBA
          uint64_t b = (sourcePixel & 0x0000ff00) >> 8;

          // Scale the R value from 8-bit to 10-bit
          r = r * (1ULL << 10) / (1ULL << 8);

          // Scale the G value from 8-bit to 10-bit
          g = g * (1ULL << 10) / (1ULL << 8);

          // Scale the B value from 8-bit to 10-bit
          b = b * (1ULL << 10) / (1ULL << 8);

          // Combine the RGBA bits into R10G10B10A2
          targetPixel = (r << 0) | (g << 10) | (b << 20) | (alpha << 30);
        }
      }
      break;
    }
    case AV_PIX_FMT_RGB565: // RGB565 is (msb)5R 6G 5B(lsb)
    {
      for (unsigned int y = 0; y < m_height; ++y)
      {
        for (unsigned int x = 0; x < m_width; ++x)
        {
          const uint16_t sourcePixel =
              reinterpret_cast<const uint16_t*>(source + y * sourceStride)[x];
          uint32_t& targetPixel = reinterpret_cast<uint32_t*>(target + y * targetStride)[x];

          // Get the 5-bit R value from RGB565
          uint64_t r = (sourcePixel & 0b1111100000000000) >> 11;

          // Get the 6-bit G value from RGB565
          uint64_t g = (sourcePixel & 0b0000011111100000) >> 5;

          // Get the 5-bit B value from RGB565
          uint64_t b = (sourcePixel & 0b0000000000011111);

          // Scale the R value from 5-bit to 10-bit
          r = r * (1ULL << 10) / (1ULL << 5);

          // Scale the G value from 6-bit to 10-bit
          g = g * (1ULL << 10) / (1ULL << 6);

          // Scale the B value from 5-bit to 10-bit
          b = b * (1ULL << 10) / (1ULL << 5);

          // Combine the RGBA bits into R10G10B10A2
          targetPixel = (r << 0) | (g << 10) | (b << 20) | (alpha << 30);
        }
      }
      break;
    }
    case AV_PIX_FMT_RGB555: // RGB555 is (msb)1X 5R 5G 5B(lsb)
    {
      for (unsigned int y = 0; y < m_height; ++y)
      {
        for (unsigned int x = 0; x < m_width; ++x)
        {
          const uint16_t sourcePixel =
              reinterpret_cast<const uint16_t*>(source + y * sourceStride)[x];
          uint32_t& targetPixel = reinterpret_cast<uint32_t*>(target + y * targetStride)[x];

          // Get the 5-bit R value from RGB555
          uint64_t r = (sourcePixel & 0b0111110000000000) >> 10;

          // Get the 5-bit G value from RGB555
          uint64_t g = (sourcePixel & 0b0000001111100000) >> 5;

          // Get the 5-bit B value from RGB555
          uint64_t b = (sourcePixel & 0b0000000000011111);

          // Scale the R value from 5-bit to 10-bit
          r = r * (1ULL << 10) / (1ULL << 5);

          // Scale the G value from 5-bit to 10-bit
          g = g * (1ULL << 10) / (1ULL << 5);

          // Scale the B value from 5-bit to 10-bit
          b = b * (1ULL << 10) / (1ULL << 5);

          // Combine the RGBA bits into R10G10B10A2
          targetPixel = (r << 0) | (g << 10) | (b << 20) | (alpha << 30);
        }
      }
      break;
    }
    default:
      break;
  }
}

AVPixelFormat CWinRenderBuffer::GetPixFormat(DXGI_FORMAT dxFormat)
{
  return AV_PIX_FMT_BGRA; //! @todo
}

// --- CWinRenderBufferPool ----------------------------------------------------

CWinRenderBufferPool::CWinRenderBufferPool()
{
  CompileOutputShaders();
}

bool CWinRenderBufferPool::IsCompatible(const CRenderVideoSettings& renderSettings) const
{
  //! @todo Move this logic to generic class

  // Shader presets are compatible
  if (!renderSettings.GetShaderPreset().empty())
    return true;

  // If no shader preset is specified, scaling methods must match
  return GetShader(renderSettings.GetScalingMethod()) != nullptr;
}

IRenderBuffer* CWinRenderBufferPool::CreateRenderBuffer(void* header /* = nullptr */)
{
  return new CWinRenderBuffer(m_format, m_targetDxFormat);
}

bool CWinRenderBufferPool::ConfigureDX(DXGI_FORMAT dxFormat)
{
  if (m_targetDxFormat != DXGI_FORMAT_UNKNOWN)
    return false; // Already configured

  m_targetDxFormat = dxFormat;

  return true;
}

SHADER::CRPWinOutputShader* CWinRenderBufferPool::GetShader(SCALINGMETHOD scalingMethod) const
{
  auto it = m_outputShaders.find(scalingMethod);

  if (it != m_outputShaders.end())
    return it->second.get();

  return nullptr;
}

const std::vector<SCALINGMETHOD>& CWinRenderBufferPool::GetScalingMethods()
{
  static std::vector<SCALINGMETHOD> scalingMethods = {
      SCALINGMETHOD::NEAREST,
      SCALINGMETHOD::LINEAR,
  };

  return scalingMethods;
}

void CWinRenderBufferPool::CompileOutputShaders()
{
  for (auto scalingMethod : GetScalingMethods())
  {
    std::unique_ptr<SHADER::CRPWinOutputShader> outputShader(new SHADER::CRPWinOutputShader);
    if (outputShader->Create(scalingMethod))
      m_outputShaders[scalingMethod] = std::move(outputShader);
    else
      CLog::Log(LOGERROR, "RPWinRenderer: Unable to create output shader ({})",
                CRenderTranslator::TranslateScalingMethod(scalingMethod));
  }
}

// --- CRPWinRenderer ----------------------------------------------------------

CRPWinRenderer::CRPWinRenderer(const CRenderSettings& renderSettings,
                               CRenderContext& context,
                               std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
  // Initialize CRPBaseRenderer fields
  m_shaderPreset.reset(new SHADER::CShaderPresetDX(m_context));
}

bool CRPWinRenderer::ConfigureInternal()
{
  CRenderSystemDX* renderingDx = static_cast<CRenderSystemDX*>(m_context.Rendering());

  DXGI_FORMAT targetDxFormat = renderingDx->GetBackBuffer().GetFormat();

  static_cast<CWinRenderBufferPool*>(m_bufferPool.get())->ConfigureDX(targetDxFormat);

  return true;
}

void CRPWinRenderer::RenderInternal(bool clear, uint8_t alpha)
{
  CRenderSystemDX* renderingDx = static_cast<CRenderSystemDX*>(m_context.Rendering());

  // Set alpha blend state
  renderingDx->SetAlphaBlendEnable(alpha < 0xFF);

  Render(renderingDx->GetBackBuffer());
}

bool CRPWinRenderer::Supports(RENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE::STRETCH || feature == RENDERFEATURE::ZOOM ||
      feature == RENDERFEATURE::PIXEL_RATIO || feature == RENDERFEATURE::ROTATION)
    return true;

  return false;
}

bool CRPWinRenderer::SupportsScalingMethod(SCALINGMETHOD method)
{
  if (method == SCALINGMETHOD::LINEAR || method == SCALINGMETHOD::NEAREST)
    return true;

  return false;
}

void CRPWinRenderer::Render(CD3DTexture& target)
{
  const CPoint destPoints[4] = {m_rotatedDestCoords[0], m_rotatedDestCoords[1],
                                m_rotatedDestCoords[2], m_rotatedDestCoords[3]};

  CWinRenderBuffer* renderBuffer = static_cast<CWinRenderBuffer*>(m_renderBuffer);
  if (renderBuffer == nullptr)
    return;

  SHADER::CShaderTextureCD3D* renderBufferTarget = renderBuffer->GetTarget();
  if (renderBufferTarget == nullptr)
    return;

  Updateshaders();

  // Are we using video shaders?
  if (m_bUseShaderPreset)
  {
    // TODO: Orientation?
    /*
    CPoint destPoints[4];
    // select destination rectangle
    if (m_renderOrientation)
    {
      for (size_t i = 0; i < 4; i++)
        destPoints[i] = m_rotatedDestCoords[i];
    }
    else
    {
      CRect destRect = m_context.StereoCorrection(m_renderSettings.Geometry().Dimensions());
      destPoints[0] = { destRect.x1, destRect.y1 };
      destPoints[1] = { destRect.x2, destRect.y1 };
      destPoints[2] = { destRect.x2, destRect.y2 };
      destPoints[3] = { destRect.x1, destRect.y2 };
    }
    */

    // Render shaders and ouput to display
    m_targetTexture.SetTexture(target);
    if (!m_shaderPreset->RenderUpdate(destPoints, renderBufferTarget, &m_targetTexture))
    {
      m_shadersNeedUpdate = false;
      m_bUseShaderPreset = false;
    }
  }
  else // Not using video shaders, output using output shader
  {
    CD3DTexture* intermediateTarget = renderBufferTarget->GetPointer();

    if (intermediateTarget != nullptr)
    {
      CRect viewPort;
      m_context.GetViewPort(viewPort);

      // Pick appropriate output shader depending on the scaling method of the renderer
      SCALINGMETHOD scalingMethod = m_renderSettings.VideoSettings().GetScalingMethod();

      CWinRenderBufferPool* bufferPool = static_cast<CWinRenderBufferPool*>(m_bufferPool.get());
      SHADER::CRPWinOutputShader* outputShader = bufferPool->GetShader(scalingMethod);

      // Use the picked output shader to render to the target
      if (outputShader != nullptr)
      {
        outputShader->Render(*intermediateTarget, m_sourceRect, destPoints, viewPort, &target,
                             m_context.UseLimitedColor() ? 1 : 0);
      }
    }
  }
}
