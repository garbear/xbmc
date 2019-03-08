/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderDX.h"
#include "ShaderTextureDX.h"
#include "Application.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTypesDX.h"
#include "cores/RetroPlayer/shaders/IShaderLut.h"
#include "cores/RetroPlayer/shaders/ShaderUtils.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "system.h"

using namespace KODI;
using namespace SHADER;

CShaderDX::CShaderDX(RETRO::CRenderContext &context) :
  m_context(context)
{
}

CShaderDX::~CShaderDX()
{
  SAFE_RELEASE(m_pInputBuffer);
}

bool CShaderDX::Create(const std::string& shaderSource, const std::string& shaderPath, ShaderParameterMap shaderParameters,
  IShaderSampler* sampler, ShaderLutVec luts, float2 viewPortSize, unsigned frameCountMod)
{
  if (shaderPath.empty())
  {
    CLog::Log(LOGERROR, "ShaderDX: Can't load empty shader path");
    return false;
  }

  m_shaderSource = shaderSource;
  m_shaderPath = shaderPath;
  m_shaderParameters = shaderParameters;
  m_pSampler = reinterpret_cast<ID3D11SamplerState*>(sampler);
  m_luts = luts;
  m_viewportSize = viewPortSize;
  m_frameCountMod = frameCountMod;

  DefinesMap defines;

  defines["HLSL_4"] = "";  // using Shader Model 4
  defines["HLSL_FX"] = "";  // and the FX11 framework

  // We implement runtime shader parameters ("#pragma parameter")
  // NOTICE: Runtime shader parameters allow convenient experimentation with real-time
  //         feedback, as well as override-ability by presets, but sometimes they are
  //         much slower because they prevent static evaluation of a lot of math.
  //         Disabling them drastically speeds up shaders that use them heavily.
  defines["PARAMETER_UNIFORM"] = "";

  m_effect.AddIncludePath(URIUtils::GetBasePath(m_shaderPath));

  if (!m_effect.Create(shaderSource, &defines))
  {
    CLog::Log(LOGERROR, "%s: failed to load video shader: %s", __FUNCTION__, shaderPath.c_str());
    return false;
  }

  return true;
}

void CShaderDX::Render(IShaderTexture* source, IShaderTexture* target)
{
  auto* sourceDX = static_cast<CShaderTextureCD3D*>(source);
  auto* targetDX = static_cast<CShaderTextureCD3D*>(target);

  // TODO: Doesn't work. Another PSSetSamplers gets called by FX11 right before rendering, overriding this
  /*
  CRenderSystemDX *renderingDx = static_cast<CRenderSystemDX*>(m_context.Rendering());
  renderingDx->Get3D11Context()->PSSetSamplers(2, 1, &m_pSampler);
  */

  SetShaderParameters( *sourceDX->GetPointer() );
  Execute({ targetDX->GetPointer() }, 4);
}

void CShaderDX::SetShaderParameters(CD3DTexture& sourceTexture)
{
  m_effect.SetTechnique("TEQ");
  m_effect.SetResources("decal", { sourceTexture.GetAddressOfSRV() }, 1);
  m_effect.SetMatrix("modelViewProj", reinterpret_cast<const float*>(&m_MVP));
  // TODO(optimization): Add frame_count to separate cbuffer
  m_effect.SetConstantBuffer("input", m_pInputBuffer);
  for (const auto& param : m_shaderParameters)
    m_effect.SetFloatArray(param.first.c_str(), &param.second, 1);
  for (const auto& lut : m_luts)
  {
    auto* texture = dynamic_cast<CShaderTextureCDX*>(lut->GetTexture());
    if (texture != nullptr)
      m_effect.SetTexture(lut->GetID().c_str(), texture->GetShaderResource());
  }
}

void CShaderDX::PrepareParameters(CPoint dest[4], bool isLastPass, uint64_t frameCount)
{
  UpdateInputBuffer(frameCount);

  CUSTOMVERTEX* v;
  LockVertexBuffer(reinterpret_cast<void**>(&v));

  if (!isLastPass)
  {
    // top left
    v[0].x = -m_outputSize.x / 2;
    v[0].y = -m_outputSize.y / 2;
    // top right
    v[1].x = m_outputSize.x / 2;
    v[1].y = -m_outputSize.y / 2;
    // bottom right
    v[2].x = m_outputSize.x / 2;
    v[2].y = m_outputSize.y / 2;
    // bottom left
    v[3].x = -m_outputSize.x / 2;
    v[3].y = m_outputSize.y / 2;
  }
  else  // last pass
  {
    // top left
    v[0].x = dest[0].x - m_outputSize.x / 2;
    v[0].y = dest[0].y - m_outputSize.y / 2;
    // top right
    v[1].x = dest[1].x - m_outputSize.x / 2;
    v[1].y = dest[1].y - m_outputSize.y / 2;
    // bottom right
    v[2].x = dest[2].x - m_outputSize.x / 2;
    v[2].y = dest[2].y - m_outputSize.y / 2;
    // bottom left
    v[3].x = dest[3].x - m_outputSize.x / 2;
    v[3].y = dest[3].y - m_outputSize.y / 2;
  }

  // top left
  v[0].z = 0;
  v[0].tu = 0;
  v[0].tv = 0;
  // top right
  v[1].z = 0;
  v[1].tu = 1;
  v[1].tv = 0;
  // bottom right
  v[2].z = 0;
  v[2].tu = 1;
  v[2].tv = 1;
  // bottom left
  v[3].z = 0;
  v[3].tu = 0;
  v[3].tv = 1;
  UnlockVertexBuffer();
}

bool CShaderDX::CreateVertexBuffer(unsigned vertCount, unsigned vertSize)
{
  return CWinShader::CreateVertexBuffer(vertCount, vertSize);
}

bool CShaderDX::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* layout, unsigned numElements)
{
  return CWinShader::CreateInputLayout(layout, numElements);
}

CD3DEffect& CShaderDX::GetEffect()
{
  return m_effect;
}

void CShaderDX::UpdateMVP()
{
  float xScale = 1.0f / m_outputSize.x * 2.0f;
  float yScale = -1.0f / m_outputSize.y * 2.0f;

  // Update projection matrix
  m_MVP = XMFLOAT4X4(
    xScale, 0, 0, 0,
    0, yScale, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  );
}

bool CShaderDX::CreateInputBuffer()
{
  CRenderSystemDX *renderingDx = static_cast<CRenderSystemDX*>(m_context.Rendering());

  ID3D11Device* pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  cbInput inputInitData = GetInputData();
  UINT inputBufSize = static_cast<UINT>((sizeof(cbInput) + 15) & ~15);
  CD3D11_BUFFER_DESC cbInputDesc(inputBufSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  D3D11_SUBRESOURCE_DATA initInputSubresource = { &inputInitData, 0, 0 };
  if (FAILED(pDevice->CreateBuffer(&cbInputDesc, &initInputSubresource, &m_pInputBuffer)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to create constant buffer for video shader input data.");
    return false;
  }

  return true;
}

void CShaderDX::UpdateInputBuffer(uint64_t frameCount)
{
  ID3D11DeviceContext1 *pContext = DX::DeviceResources::Get()->GetD3DContext();

  cbInput input = GetInputData(frameCount);
  cbInput* pData;
  void** ppData = reinterpret_cast<void**>(&pData);

  D3D11_MAPPED_SUBRESOURCE resource;
  if (SUCCEEDED(pContext->Map(m_pInputBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)))
  {
    *ppData = resource.pData;

    memcpy(*ppData, &input, sizeof(cbInput));
    pContext->Unmap(m_pInputBuffer, 0);
  }
}

CShaderDX::cbInput CShaderDX::GetInputData(uint64_t frameCount)
{
  if (m_frameCountMod != 0)
    frameCount %= m_frameCountMod;

  cbInput input = {
    // Resution of texture passed to the shader
    { m_inputSize.ToDXVector() },       // video_size
    // Shaders don't (and shouldn't) know about _actual_ texture
    // size, because D3D gives them correct texture coordinates
    { m_inputSize.ToDXVector() },       // texture_size
    // As per the spec, this is the viewport resolution (not the
    // output res of each shader
    { m_viewportSize.ToDXVector() },    // output_size
    // Current frame count that can be modulo'ed
    { static_cast<float>(frameCount) }, // frame_count
    // Time always flows forward
    { 1.0f }                            // frame_direction
  };

  return input;
}

void CShaderDX::SetSizes(const float2& prevSize, const float2& nextSize)
{
  m_inputSize = prevSize;
  m_outputSize = nextSize;
}
