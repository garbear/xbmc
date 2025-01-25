/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetDX.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/IShaderSampler.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "cores/RetroPlayer/shaders/ShaderUtils.h"
#include "cores/RetroPlayer/shaders/windows/ShaderDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderLutDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTextureDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTypesDX.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <regex>

using namespace KODI;
using namespace SHADER;

CShaderPresetDX::CShaderPresetDX(RETRO::CRenderContext& context,
                                 unsigned videoWidth,
                                 unsigned videoHeight)
  : m_context(context), m_videoSize(videoWidth, videoHeight)
{
  CRect viewPort;
  m_context.GetViewPort(viewPort);
  m_outputSize = {viewPort.Width(), viewPort.Height()};
}

CShaderPresetDX::~CShaderPresetDX()
{
  DisposeShaders();

  // The GUI is going to render after this, so apply the state required
  m_context.ApplyStateBlock();
}

bool CShaderPresetDX::ReadPresetFile(const std::string& presetPath)
{
  return CServiceBroker::GetGameServices().VideoShaders().LoadPreset(presetPath, *this);
}

bool CShaderPresetDX::RenderUpdate(const CPoint dest[],
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

  PrepareParameters(dest, source, target);

  const unsigned int numPasses = static_cast<unsigned int>(m_pShaders.size());

  // Apply all passes except the last one (which needs to be applied to the backbuffer)
  for (unsigned shaderIdx = 0; shaderIdx + 1 < numPasses; ++shaderIdx)
  {
    IShader* shader = m_pShaders[shaderIdx].get();
    IShaderTexture* texture = m_pShaderTextures[shaderIdx].get();
    RenderShader(shader, source, texture);
    source = texture;
  }

  // Restore our viewport
  m_context.SetViewPort(viewPort);
  m_context.SetScissors(viewPort);

  // Apply the last pass and write to target (backbuffer) instead of the last texture
  IShader* lastShader = m_pShaders.back().get();
  lastShader->Render(source, target);

  m_frameCount += static_cast<float>(m_speed);
  return true;
}

void CShaderPresetDX::SetVideoSize(unsigned int videoWidth, unsigned int videoHeight)
{
  if (videoWidth != static_cast<unsigned int>(m_videoSize.x) ||
      videoHeight != static_cast<unsigned int>(m_videoSize.y))
  {
    m_videoSize = {videoWidth, videoHeight};
    m_bPresetNeedsUpdate = true;
  }
}

bool CShaderPresetDX::SetShaderPreset(const std::string& shaderPresetPath)
{
  m_bPresetNeedsUpdate = true;
  m_presetPath = shaderPresetPath;
  return Update();
}

const std::string& CShaderPresetDX::GetShaderPreset() const
{
  return m_presetPath;
}

bool CShaderPresetDX::Update()
{
  auto updateFailed = [this](const std::string& msg)
  {
    m_failedPaths.insert(m_presetPath);
    CLog::Log(LOGWARNING, "CShaderPresetDX::Update: {}", msg);
    DisposeShaders();
    return false;
  };

  if (m_bPresetNeedsUpdate && !HasPathFailed(m_presetPath))
  {
    DisposeShaders();

    if (m_presetPath.empty())
      // No preset should load, just return false, we shouldn't add "" to the failed paths
      return false;

    if (!ReadPresetFile(m_presetPath))
    {
      CLog::LogF(LOGERROR, "Couldn't load shader preset {} or the shaders it references",
                 m_presetPath);
      return false;
    }

    if (!CreateShaders())
      return updateFailed("Failed to initialize shaders");

    if (!CreateLayouts())
      return updateFailed("Failed to create layouts");

    if (!CreateBuffers())
      return updateFailed("Failed to initialize buffers");

    if (!CreateShaderTextures())
      return updateFailed("A shader texture failed to init");

    if (!CreateSamplers())
      return updateFailed("Failed to create samplers");
  }

  if (m_pShaders.empty())
    return false;

  // Each pass except the last one must have its own texture and the opposite is also true
  if (m_pShaders.size() != m_pShaderTextures.size() + 1)
    return updateFailed("A shader or texture failed to init");

  m_bPresetNeedsUpdate = false;
  return true;
}

void CShaderPresetDX::UpdateViewPort()
{
  CRect viewPort;
  m_context.GetViewPort(viewPort);
  UpdateViewPort(viewPort);
}

void CShaderPresetDX::UpdateViewPort(CRect viewPort)
{
  const float2 currentViewPortSize = {viewPort.Width(), viewPort.Height()};
  if (currentViewPortSize != m_outputSize)
  {
    m_outputSize = currentViewPortSize;
    m_bPresetNeedsUpdate = true;
    Update();
  }
}

void CShaderPresetDX::UpdateMVPs()
{
  for (std::unique_ptr<IShader>& videoShader : m_pShaders)
    videoShader->UpdateMVP();
}

void CShaderPresetDX::PrepareParameters(const CPoint dest[],
                                        IShaderTexture* source,
                                        IShaderTexture* target)
{
  if (m_dest[0] != dest[0] || m_dest[1] != dest[1] || m_dest[2] != dest[2] ||
      m_dest[3] != dest[3] || target->GetWidth() != m_outputSize.x ||
      target->GetHeight() != m_outputSize.y)
  {
    for (size_t i = 0; i < 4; ++i)
      m_dest[i] = dest[i];

    m_outputSize = {target->GetWidth(), target->GetHeight()};

    // Update projection matrix and update video shaders
    UpdateMVPs();
    UpdateViewPort();
  }

  const unsigned int numPasses = static_cast<unsigned int>(m_pShaders.size());

  // Prepare parameters for all shader passes
  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    std::unique_ptr<IShader>& videoShader = m_pShaders[shaderIdx];
    videoShader->PrepareParameters(m_dest, source, m_pShaderTextures, m_pShaders,
                                   static_cast<uint64_t>(m_frameCount));
  }
}

bool CShaderPresetDX::CreateShaders()
{
  const unsigned int numPasses = static_cast<unsigned int>(m_passes.size());

  //! @todo Is this pass specific?
  std::vector<std::shared_ptr<IShaderLut>> passLUTsDX;
  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const ShaderPass& pass = m_passes[shaderIdx];
    const unsigned int numPassLuts = static_cast<unsigned int>(pass.luts.size());

    for (unsigned int i = 0; i < numPassLuts; ++i)
    {
      const ShaderLut& lutStruct = pass.luts[i];

      std::shared_ptr<CShaderLutDX> passLut =
          std::make_shared<CShaderLutDX>(lutStruct.strId, lutStruct.path);
      if (passLut->Create(m_context, lutStruct))
        passLUTsDX.emplace_back(std::move(passLut));
    }

    // Create the shader
    std::unique_ptr<CShaderDX> videoShader = std::make_unique<CShaderDX>(m_context);

    const std::string& shaderSource = pass.vertexSource; // Also contains fragment source
    const std::string& shaderPath = pass.sourcePath;

    // Get only the parameters belonging to this specific shader
    ShaderParameterMap passParameters = GetShaderParameters(pass.parameters, pass.vertexSource);

    if (!videoShader->Create(shaderSource, shaderPath, std::move(passParameters), std::move(passLUTsDX), m_outputSize, shaderIdx, pass.frameCountMod))
    {
      CLog::Log(LOGERROR, "Couldn't create a video shader");
      return false;
    }
    m_pShaders.push_back(std::move(videoShader));

    /*
    IShaderSampler* passSampler = reinterpret_cast<IShaderSampler*>(
        pass.filter == FILTER_TYPE_LINEAR
            ? m_pSampLinear
            : m_pSampNearest); //! @todo Wrap in CShaderSamplerDX instead of reinterpret_cast

    //! @todo Set passSampler to m_pSampler in the videoShader
    videoShader->SetSampler(passSampler);
    */
  }

  return true;
}

bool CShaderPresetDX::CreateLayouts()
{
  for (std::unique_ptr<IShader>& videoShader : m_pShaders)
  {
    CShaderDX* videoShaderDX = static_cast<CShaderDX*>(videoShader.get());
    videoShaderDX->CreateVertexBuffer(4, sizeof(CUSTOMVERTEX));

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0}};

    if (!videoShaderDX->CreateInputLayout(layout, ARRAYSIZE(layout)))
    {
      CLog::Log(LOGERROR, __FUNCTION__ ": Failed to create input layout for Input Assembler.");
      return false;
    }
  }

  return true;
}

bool CShaderPresetDX::CreateBuffers()
{
  for (std::unique_ptr<IShader>& videoShader : m_pShaders)
  {
    CShaderDX* videoShaderDX = static_cast<CShaderDX*>(videoShader.get());
    videoShaderDX->CreateInputBuffer();
  }

  return true;
}

bool CShaderPresetDX::CreateShaderTextures()
{
  m_pShaderTextures.clear();

  float2 prevSize = m_videoSize;
  float2 prevTextureSize = m_videoSize;

  const unsigned int numPasses = static_cast<unsigned int>(m_passes.size());

  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const ShaderPass& pass = m_passes[shaderIdx];

    // Resolve final texture resolution, taking scale type and scale multiplier into account
    float2 scaledSize;
    float2 textureSize;
    switch (pass.fbo.scaleX.scaleType)
    {
      case ScaleType::ABSOLUTE_SCALE:
        scaledSize.x = static_cast<float>(pass.fbo.scaleX.abs);
        break;
      case ScaleType::VIEWPORT:
        scaledSize.x =
            pass.fbo.scaleX.scale ? pass.fbo.scaleX.scale * m_outputSize.x : m_outputSize.x;
        break;
      case ScaleType::INPUT:
      default:
        scaledSize.x = pass.fbo.scaleX.scale ? pass.fbo.scaleX.scale * prevSize.x : prevSize.x;
        break;
    }
    switch (pass.fbo.scaleY.scaleType)
    {
      case ScaleType::ABSOLUTE_SCALE:
        scaledSize.y = static_cast<float>(pass.fbo.scaleY.abs);
        break;
      case ScaleType::VIEWPORT:
        scaledSize.y =
            pass.fbo.scaleY.scale ? pass.fbo.scaleY.scale * m_outputSize.y : m_outputSize.y;
        break;
      case ScaleType::INPUT:
      default:
        scaledSize.y = pass.fbo.scaleY.scale ? pass.fbo.scaleY.scale * prevSize.y : prevSize.y;
        break;
    }

    if (shaderIdx + 1 == numPasses)
    {
      // We're supposed to output at full (viewport) resolution
      scaledSize.x = m_outputSize.x;
      scaledSize.y = m_outputSize.y;
    }
    else
    {
      // Determine the framebuffer data format
      DXGI_FORMAT textureFormat;
      if (pass.fbo.floatFramebuffer)
      {
        // Give priority to float framebuffer parameter (we can't use both float and sRGB)
        textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
      }
      else
      {
        if (pass.fbo.sRgbFramebuffer)
          textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        else
          textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
      }

      //! @todo Enable usage of optimal texture sizes when all issues are fixed
      textureSize = scaledSize; // CShaderUtils::GetOptimalTextureSize(scaledSize)

      CD3DTexture* textureDX = new CD3DTexture();

      if (!textureDX->Create(static_cast<UINT>(textureSize.x), static_cast<UINT>(textureSize.y), 1,
                             D3D11_USAGE_DEFAULT, textureFormat, nullptr, 0))
      {
        CLog::Log(LOGERROR, "Couldn't create a texture for video shader {}", pass.sourcePath);
        return false;
      }

      m_pShaderTextures.emplace_back(std::make_unique<CShaderTextureCD3D>(textureDX));
    }

    // Notify shader of its source and dest size
    m_pShaders[shaderIdx]->SetSizes(prevSize, prevTextureSize, scaledSize);

    prevSize = scaledSize;
    prevTextureSize = textureSize;
  }

  UpdateMVPs();
  return true;
}

bool CShaderPresetDX::CreateSamplers()
{
  // Describe the Sampler States
  // As specified in the common-shaders spec
  D3D11_SAMPLER_DESC sampDesc;
  ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
  sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampDesc.MinLOD = 0;
  sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
  FLOAT blackBorder[4] = {1, 0, 0, 1}; //! @todo Turn this back to black
  memcpy(sampDesc.BorderColor, &blackBorder, 4 * sizeof(FLOAT));

  ID3D11Device1* pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (FAILED(pDevice->CreateSamplerState(&sampDesc, &m_pSampNearest)))
    return false;

  D3D11_SAMPLER_DESC sampDescLinear = sampDesc;
  sampDescLinear.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  if (FAILED(pDevice->CreateSamplerState(&sampDescLinear, &m_pSampLinear)))
    return false;

  return true;
}

void CShaderPresetDX::RenderShader(IShader* shader,
                                   IShaderTexture* source,
                                   IShaderTexture* target) const
{
  const CRect newViewPort(0.f, 0.f, target->GetWidth(), target->GetHeight());
  m_context.SetViewPort(newViewPort);
  m_context.SetScissors(newViewPort);

  shader->Render(source, target);
}

void CShaderPresetDX::DisposeShaders()
{
  m_pShaders.clear();
  m_pShaderTextures.clear();
  m_passes.clear();
  m_bPresetNeedsUpdate = true;
}

bool CShaderPresetDX::HasPathFailed(const std::string& path) const
{
  return m_failedPaths.find(path) != m_failedPaths.end();
}

ShaderParameterMap CShaderPresetDX::GetShaderParameters(
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
  for (const std::string& match : validParams)
  {
    // For each param found in the preset file
    for (const ShaderParameter& parameter : parameters)
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
