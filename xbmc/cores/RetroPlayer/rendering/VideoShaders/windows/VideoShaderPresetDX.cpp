/*
*      Copyright (C) 2017 Team Kodi
 *     http://kodi.tv
 *
 * This Program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this Program; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoShaderPresetDX.h"

#include <regex>

#include "addons/binary-addons/BinaryAddonBase.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/VideoShaders/windows/VideoShaderDX.h"
#include "cores/RetroPlayer/rendering/VideoShaders/windows/VideoShaderTextureDX.h"
#include "cores/RetroPlayer/rendering/VideoShaders/windows/VideoShaderUtilsDX.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/log.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace KODI;
using namespace SHADER;

CVideoShaderPresetDX::CVideoShaderPresetDX(RETRO::CRenderContext &context, unsigned videoWidth, unsigned videoHeight)
  : m_context(context)
  , m_videoSize(videoWidth, videoHeight)
{
  m_textureSize = CVideoShaderUtils::GetOptimalTextureSize(m_videoSize);

  CRect viewPort;
  m_context.GetViewPort(viewPort);
  m_outputSize = { viewPort.Width(), viewPort.Height() };
}

ShaderParameters CVideoShaderPresetDX::GetShaderParameters(const std::vector<VideoShaderParameter> &parameters, const std::string& sourceStr) const
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

  ShaderParameters matchParams;
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

CVideoShaderPresetDX::~CVideoShaderPresetDX()
{
  DisposeVideoShaders();
  // The gui is going to render after this, so apply the state required
  m_context.ApplyStateBlock();
}

bool CVideoShaderPresetDX::ReadPresetFile(const std::string& presetPath)
{
  return CServiceBroker::GetGameServices().VideoShaders().LoadPreset(presetPath, *this);
}

bool CVideoShaderPresetDX::RenderUpdate(const CPoint dest[], IShaderTexture* source, IShaderTexture* target)
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

  IVideoShader* firstShader = m_pVideoShaders.front().get();
  CShaderTextureCD3D* firstShaderTexture = m_pShaderTextures.front().get();
  IVideoShader* lastShader = m_pVideoShaders.back().get();

  const unsigned passesNum = m_pShaderTextures.size();

  if (passesNum == 1)
    m_pVideoShaders.front()->Render(source, target);
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
    for (unsigned shaderIdx = 1; shaderIdx < m_pVideoShaders.size() - 1; ++shaderIdx)
    {
      IVideoShader* shader = m_pVideoShaders[shaderIdx].get();
      CShaderTextureCD3D* prevTexture = m_pShaderTextures[shaderIdx - 1].get();
      CShaderTextureCD3D* texture = m_pShaderTextures[shaderIdx].get();
      RenderShader(shader, prevTexture, texture);
    }

    // Apply last pass and write to target (backbuffer) instead of the last texture
    CShaderTextureCD3D* secToLastTexture = m_pShaderTextures[m_pShaderTextures.size() - 2].get();
    RenderShader(lastShader, secToLastTexture, target);
  }

  m_frameCount += m_speed;

  // Restore our view port.
  m_context.SetViewPort(viewPort);

  return true;
}

void CVideoShaderPresetDX::RenderShader(IVideoShader* shader, IShaderTexture* source, IShaderTexture* target) const
{
  CRect newViewPort(0.f, 0.f, target->GetWidth(), target->GetHeight());
  m_context.SetViewPort(newViewPort);
  m_context.SetScissors(newViewPort);

  shader->Render(source, target);
}

bool CVideoShaderPresetDX::Update()
{
  auto updateFailed = [this](const std::string& msg)
  {
    m_failedPaths.insert(m_presetPath);
    auto message = "CVideoShaderPresetDX::Update: " + msg + ". Disabling video shaders.";
    CLog::Log(LOGWARNING, message.c_str());
    DisposeVideoShaders();
    return false;
  };

  if (m_bPresetNeedsUpdate && !HasPathFailed(m_presetPath))
  {
    DisposeVideoShaders();

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

  if (m_pVideoShaders.empty())
    return false;

  // Each pass must have its own texture and the opposite is also true
  if (m_pVideoShaders.size() != m_pShaderTextures.size())
    return updateFailed("A shader or texture failed to init");

  m_bPresetNeedsUpdate = false;
  return true;
}

bool CVideoShaderPresetDX::CreateShaderTextures()
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

  auto numPasses = m_passes.size();

  for (unsigned shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    auto& pass = m_passes[shaderIdx];

    // resolve final texture resolution, taking scale type and scale multiplier into account

    float2 scaledSize;
    switch (pass.fbo.scaleX.type)
    {
    case SCALE_TYPE_ABSOLUTE:
      scaledSize.x = pass.fbo.scaleX.abs;
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
      scaledSize.y = pass.fbo.scaleY.abs;
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
    if (!texture->Create(scaledSize.x, scaledSize.y, 1, D3D11_USAGE_DEFAULT, textureFormat, nullptr, 0))
    {
      CLog::Log(LOGERROR, "Couldn't create a texture for video shader %s.", pass.sourcePath.c_str());
      return false;
    }
    m_pShaderTextures.emplace_back(new CShaderTextureCD3D(texture));

    // notify shader of its source and dest size
    m_pVideoShaders[shaderIdx]->SetSizes(prevSize, scaledSize);

    prevSize = scaledSize;
  }
  return true;
}

bool CVideoShaderPresetDX::CreateShaders()
{
  auto numPasses = m_passes.size();
  // todo: replace with per-shader texture size
  // todo: actually use this
  m_textureSize = CVideoShaderUtils::GetOptimalTextureSize(m_videoSize);

  // todo: is this pass specific?
  IShaderLuts passLUTsDX;
  for (unsigned shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const auto& pass = m_passes[shaderIdx];

    for (unsigned i = 0; i < pass.luts.size(); ++i)
    {
      auto& lutStruct = pass.luts[i];

      IShaderSampler* lutSampler(CreateLUTSampler(m_context, lutStruct));
      IShaderTexture* lutTexture(CreateLUTexture(lutStruct));

      if (!lutSampler || !lutTexture)
      {
        CLog::Log(LOGWARNING, "%s - Couldn't create a LUT sampler or texture for LUT %s", __FUNCTION__, lutStruct.strId);
        return false;
      }
      else
        passLUTsDX.emplace_back(new ShaderLutDX(lutStruct.strId, lutStruct.path, lutSampler, lutTexture));
    }

    // For reach pass, create the shader
    std::unique_ptr<CVideoShaderDX> videoShader(new CVideoShaderDX(m_context));

    auto shaderSrc = pass.vertexSource; // also contains fragment source
    auto shaderPath = pass.sourcePath;

    // Get only the parameters belonging to this specific shader
    ShaderParameters passParameters = GetShaderParameters(pass.parameters, pass.vertexSource);
    IShaderSampler* passSampler = reinterpret_cast<IShaderSampler*>(pass.filter ? m_pSampLinear : m_pSampNearest);

    if (!videoShader->Create(shaderSrc, shaderPath, passParameters, passSampler, passLUTsDX, m_outputSize, pass.frameCountMod))
    {
      CLog::Log(LOGERROR, "Couldn't create a video shader");
      return false;
    }
    m_pVideoShaders.push_back(std::move(videoShader));
  }
  return true;
}

bool CVideoShaderPresetDX::CreateSamplers()
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

bool CVideoShaderPresetDX::CreateLayouts()
{
  for (auto& videoShader : m_pVideoShaders)
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

bool CVideoShaderPresetDX::CreateBuffers()
{
  for (auto& videoShader : m_pVideoShaders)
    videoShader->CreateInputBuffer();
  return true;
}

void CVideoShaderPresetDX::PrepareParameters(const IShaderTexture* texture, const CPoint dest[])
{
  // prepare params for all shaders except the last (needs special flag)
  for (unsigned shaderIdx = 0; shaderIdx < m_pVideoShaders.size() - 1; ++shaderIdx)
  {
    auto& videoShader = m_pVideoShaders[shaderIdx];
    videoShader->PrepareParameters(m_dest, false, m_frameCount);
  }

  // prepare params for last shader
  m_pVideoShaders.back()->PrepareParameters(m_dest, true, m_frameCount);

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

bool CVideoShaderPresetDX::HasPathFailed(const std::string& path) const
{
  return m_failedPaths.find(path) != m_failedPaths.end();
}

void CVideoShaderPresetDX::DisposeVideoShaders()
{
  //firstTexture.reset();
  m_pVideoShaders.clear();
  m_pShaderTextures.clear();
  m_passes.clear();
  m_bPresetNeedsUpdate = true;
}

//CShaderTextureDX* CVideoShaderPresetDX::GetFirstTexture()
//{
//  return firstTexture.get();
//}

bool CVideoShaderPresetDX::SetShaderPreset(const std::string& shaderPresetPath)
{
  m_bPresetNeedsUpdate = true;
  m_presetPath = shaderPresetPath;
  return Update();
}

const std::string& CVideoShaderPresetDX::GetShaderPreset() const
{
  return m_presetPath;
}

void CVideoShaderPresetDX::SetVideoSize(const unsigned videoWidth, const unsigned videoHeight)
{
  m_videoSize = { videoWidth, videoHeight };
  m_textureSize = CVideoShaderUtils::GetOptimalTextureSize(m_videoSize);
}

void CVideoShaderPresetDX::UpdateMVPs()
{
  for (auto& videoShader : m_pVideoShaders)
    videoShader->UpdateMVP();
}

void CVideoShaderPresetDX::UpdateViewPort()
{
  CRect viewPort;
  m_context.GetViewPort(viewPort);
  UpdateViewPort(viewPort);
}

void CVideoShaderPresetDX::UpdateViewPort(CRect viewPort)
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
