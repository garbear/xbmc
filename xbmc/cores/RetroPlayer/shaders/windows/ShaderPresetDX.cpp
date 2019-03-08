/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetDX.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/windows/ShaderDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderLutDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTextureDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTypesDX.h"
#include "cores/RetroPlayer/shaders/IShaderSampler.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "cores/RetroPlayer/shaders/ShaderUtils.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "ServiceBroker.h"

#include <regex>

using namespace KODI;
using namespace SHADER;

CShaderPresetDX::CShaderPresetDX(RETRO::CRenderContext &context, unsigned videoWidth, unsigned videoHeight)
  : m_context(context)
  , m_videoSize(videoWidth, videoHeight)
{
  m_textureSize = CShaderUtils::GetOptimalTextureSize(m_videoSize);

  CRect viewPort;
  m_context.GetViewPort(viewPort);
  m_outputSize = { viewPort.Width(), viewPort.Height() };
}

ShaderParameterMap CShaderPresetDX::GetShaderParameters(const std::vector<ShaderParameter> &parameters, const std::string& sourceStr) const
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
  for (const auto& match : validParams)   // for each param found in the source code
  {
    for (const auto& parameter : parameters)   // for each param found in the preset file
    {
      if (match == parameter.strId)  // if they match
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

CShaderPresetDX::~CShaderPresetDX()
{
  DisposeShaders();
  // The gui is going to render after this, so apply the state required
  m_context.ApplyStateBlock();
}

bool CShaderPresetDX::ReadPresetFile(const std::string& presetPath)
{
  return CServiceBroker::GetGameServices().VideoShaders().LoadPreset(presetPath, *this);
}

bool CShaderPresetDX::RenderUpdate(const CPoint dest[], IShaderTexture* source, IShaderTexture* target)
{
  // Save the viewport
  CRect viewPort;
  m_context.GetViewPort(viewPort);

  // Handle resizing of the viewport (window)
  UpdateViewPort(viewPort);

  // Update shaders/shader textures if required
  if (!Update())
    return false;

  PrepareParameters(target, dest);

  // At this point, the input video has been rendered to the first texture ("source", not m_pShaderTextures[0])

  IShader* firstShader = m_pShaders.front().get();
  CShaderTextureCD3D* firstShaderTexture = m_pShaderTextures.front().get();
  IShader* lastShader = m_pShaders.back().get();

  const unsigned passesNum = static_cast<unsigned int>(m_pShaderTextures.size());

  if (passesNum == 1)
    m_pShaders.front()->Render(source, target);
  else if (passesNum == 2)
  {
    // Apply first pass
    RenderShader(firstShader, source, firstShaderTexture);
    // Apply last pass
    RenderShader(lastShader, firstShaderTexture, target);
  }
  else
  {
    // Apply first pass
    RenderShader(firstShader, source, firstShaderTexture);

    // Apply all passes except the first and last one (which needs to be applied to the backbuffer)
    for (unsigned int shaderIdx = 1;
      shaderIdx < static_cast<unsigned int>(m_pShaders.size()) - 1;
      ++shaderIdx)
    {
      IShader* shader = m_pShaders[shaderIdx].get();
      CShaderTextureCD3D* prevTexture = m_pShaderTextures[shaderIdx - 1].get();
      CShaderTextureCD3D* texture = m_pShaderTextures[shaderIdx].get();
      RenderShader(shader, prevTexture, texture);
    }

    // Apply last pass and write to target (backbuffer) instead of the last texture
    CShaderTextureCD3D* secToLastTexture = m_pShaderTextures[m_pShaderTextures.size() - 2].get();
    RenderShader(lastShader, secToLastTexture, target);
  }

  m_frameCount += static_cast<float>(m_speed);

  // Restore our view port.
  m_context.SetViewPort(viewPort);

  return true;
}

void CShaderPresetDX::RenderShader(IShader* shader, IShaderTexture* source, IShaderTexture* target) const
{
  CRect newViewPort(0.f, 0.f, target->GetWidth(), target->GetHeight());
  m_context.SetViewPort(newViewPort);
  m_context.SetScissors(newViewPort);

  shader->Render(source, target);
}

bool CShaderPresetDX::Update()
{
  auto updateFailed = [this](const std::string& msg)
  {
    m_failedPaths.insert(m_presetPath);
    auto message = "CShaderPresetDX::Update: " + msg + ". Disabling video shaders.";
    CLog::Log(LOGWARNING, message.c_str());
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
      CLog::Log(LOGERROR, "%s - couldn't load shader preset %s or the shaders it references", __FUNCTION__, m_presetPath.c_str());
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

  // Each pass must have its own texture and the opposite is also true
  if (m_pShaders.size() != m_pShaderTextures.size())
    return updateFailed("A shader or texture failed to init");

  m_bPresetNeedsUpdate = false;
  return true;
}

bool CShaderPresetDX::CreateShaderTextures()
{
  m_pShaderTextures.clear();

  //CD3DTexture* fTexture(new CD3DTexture());
  //auto res = fTexture->Create(m_outputSize.x, m_outputSize.y, 1,
  //  D3D11_USAGE_DEFAULT, DXGI_FORMAT_B8G8R8A8_UNORM, nullptr, 0);
  //if (!res)
  //{
  //  CLog::Log(LOGERROR, "Couldn't create a intial video shader texture");
  //  return false;
  //}
  //firstTexture.reset(new CShaderTextureDX(fTexture));

  float2 prevSize = m_videoSize;

  unsigned int numPasses = static_cast<unsigned int>(m_passes.size());

  for (unsigned shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    ShaderPass& pass = m_passes[shaderIdx];

    // resolve final texture resolution, taking scale type and scale multiplier into account

    float2 scaledSize;
    switch (pass.fbo.scaleX.type)
    {
    case SCALE_TYPE_ABSOLUTE:
      scaledSize.x = static_cast<float>(pass.fbo.scaleX.abs);
      break;
    case SCALE_TYPE_VIEWPORT:
      scaledSize.x = m_outputSize.x;
      break;
    case SCALE_TYPE_INPUT:
    default:
      scaledSize.x = prevSize.x;
      break;
    }
    switch (pass.fbo.scaleY.type)
    {
    case SCALE_TYPE_ABSOLUTE:
      scaledSize.y = static_cast<float>(pass.fbo.scaleY.abs);
      break;
    case SCALE_TYPE_VIEWPORT:
      scaledSize.y = m_outputSize.y;
      break;
    case SCALE_TYPE_INPUT:
    default:
      scaledSize.y = prevSize.y;
      break;
    }

    // if the scale was unspecified
    if (pass.fbo.scaleX.scale == 0 && pass.fbo.scaleY.scale == 0)
    {
      // if the last shader has the scale unspecified
      if (shaderIdx == numPasses - 1)
      {
        // we're supposed to output at full (viewport) res
        // TODO: Thus, we can also (maybe) bypass rendering to an intermediate render target (on the last pass)
        scaledSize.x = m_outputSize.x;
        scaledSize.y = m_outputSize.y;
      }
    }
    else
    {
      scaledSize.x *= pass.fbo.scaleX.scale;
      scaledSize.y *= pass.fbo.scaleY.scale;
    }

    // For reach pass, create the texture

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

    CD3DTexture* texture(new CD3DTexture());
    if (!texture->Create(
        static_cast<UINT>(scaledSize.x),
        static_cast<UINT>(scaledSize.y),
        1,
        D3D11_USAGE_DEFAULT,
        textureFormat,
        nullptr,
        0))
    {
      CLog::Log(LOGERROR, "Couldn't create a texture for video shader %s.", pass.sourcePath.c_str());
      return false;
    }
    m_pShaderTextures.emplace_back(new CShaderTextureCD3D(texture));

    // notify shader of its source and dest size
    m_pShaders[shaderIdx]->SetSizes(prevSize, scaledSize);

    prevSize = scaledSize;
  }
  return true;
}

bool CShaderPresetDX::CreateShaders()
{
  auto numPasses = m_passes.size();
  // todo: replace with per-shader texture size
  // todo: actually use this
  m_textureSize = CShaderUtils::GetOptimalTextureSize(m_videoSize);

  // todo: is this pass specific?
  ShaderLutVec passLUTsDX;
  for (unsigned shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const auto& pass = m_passes[shaderIdx];

    for (unsigned i = 0; i < pass.luts.size(); ++i)
    {
      auto& lutStruct = pass.luts[i];

      ShaderLutPtr passLut(new CShaderLutDX(lutStruct.strId, lutStruct.path));
      if (passLut->Create(m_context, lutStruct))
        passLUTsDX.emplace_back(std::move(passLut));
    }

    // For reach pass, create the shader
    std::unique_ptr<CShaderDX> videoShader(new CShaderDX(m_context));

    auto shaderSrc = pass.vertexSource; // also contains fragment source
    auto shaderPath = pass.sourcePath;

    // Get only the parameters belonging to this specific shader
    ShaderParameterMap passParameters = GetShaderParameters(pass.parameters, pass.vertexSource);
    IShaderSampler* passSampler = reinterpret_cast<IShaderSampler*>(pass.filter ? m_pSampLinear : m_pSampNearest); //! @todo Wrap in CShaderSamplerDX instead of reinterpret_cast

    if (!videoShader->Create(shaderSrc, shaderPath, passParameters, passSampler, passLUTsDX, m_outputSize, pass.frameCountMod))
    {
      CLog::Log(LOGERROR, "Couldn't create a video shader");
      return false;
    }
    m_pShaders.push_back(std::move(videoShader));
  }
  return true;
}

bool CShaderPresetDX::CreateSamplers()
{
  CRenderSystemDX *renderingDx = static_cast<CRenderSystemDX*>(m_context.Rendering());

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
  FLOAT blackBorder[4] = { 1, 0, 0, 1 };  // TODO: turn this back to black
  memcpy(sampDesc.BorderColor, &blackBorder, 4 * sizeof(FLOAT));

  ID3D11Device* pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (FAILED(pDevice->CreateSamplerState(&sampDesc, &m_pSampNearest)))
    return false;

  D3D11_SAMPLER_DESC sampDescLinear = sampDesc;
  sampDescLinear.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  if (FAILED(pDevice->CreateSamplerState(&sampDescLinear, &m_pSampLinear)))
    return false;

  return true;
}

bool CShaderPresetDX::CreateLayouts()
{
  for (auto& videoShader : m_pShaders)
  {
    videoShader->CreateVertexBuffer(4, sizeof(CUSTOMVERTEX));
    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
      { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    if (!videoShader->CreateInputLayout(layout, ARRAYSIZE(layout)))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to create input layout for Input Assembler.");
      return false;
    }
  }
  return true;
}

bool CShaderPresetDX::CreateBuffers()
{
  for (auto& videoShader : m_pShaders)
    videoShader->CreateInputBuffer();
  return true;
}

void CShaderPresetDX::PrepareParameters(const IShaderTexture* texture, const CPoint dest[])
{
  // prepare params for all shaders except the last (needs special flag)
  for (unsigned shaderIdx = 0; shaderIdx < m_pShaders.size() - 1; ++shaderIdx)
  {
    auto& videoShader = m_pShaders[shaderIdx];
    videoShader->PrepareParameters(m_dest, false, static_cast<uint64_t>(m_frameCount));
  }

  // prepare params for last shader
  m_pShaders.back()->PrepareParameters(m_dest, true, static_cast<uint64_t>(m_frameCount));

  if (m_dest[0] != dest[0] || m_dest[1] != dest[1]
    || m_dest[2] != dest[2] || m_dest[3] != dest[3]
    || texture->GetWidth() != m_outputSize.x
    || texture->GetHeight() != m_outputSize.y)
  {
    for (size_t i = 0; i < 4; ++i)
      m_dest[i] = dest[i];

    m_outputSize = { texture->GetWidth(), texture->GetHeight() };

    // Update projection matrix and update video shaders
    UpdateMVPs();
    UpdateViewPort();
  }
}

bool CShaderPresetDX::HasPathFailed(const std::string& path) const
{
  return m_failedPaths.find(path) != m_failedPaths.end();
}

void CShaderPresetDX::DisposeShaders()
{
  //firstTexture.reset();
  m_pShaders.clear();
  m_pShaderTextures.clear();
  m_passes.clear();
  m_bPresetNeedsUpdate = true;
}

//CShaderTextureDX* CShaderPresetDX::GetFirstTexture()
//{
//  return firstTexture.get();
//}

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

void CShaderPresetDX::SetVideoSize(const unsigned videoWidth, const unsigned videoHeight)
{
  m_videoSize = { videoWidth, videoHeight };
  m_textureSize = CShaderUtils::GetOptimalTextureSize(m_videoSize);
}

void CShaderPresetDX::UpdateMVPs()
{
  for (auto& videoShader : m_pShaders)
    videoShader->UpdateMVP();
}

void CShaderPresetDX::UpdateViewPort()
{
  CRect viewPort;
  m_context.GetViewPort(viewPort);
  UpdateViewPort(viewPort);
}

void CShaderPresetDX::UpdateViewPort(CRect viewPort)
{
  float2 currentViewPortSize = { viewPort.Width(), viewPort.Height() };
  if (currentViewPortSize != m_outputSize)
  {
    m_outputSize = currentViewPortSize;
    //CreateShaderTextures();
    // Just re-make everything. Else we get resizing bugs.
    // Could probably refine that to only rebuild certain things, for a tiny bit of perf. (only when resizing)
    m_bPresetNeedsUpdate = true;
    Update();
  }
}
