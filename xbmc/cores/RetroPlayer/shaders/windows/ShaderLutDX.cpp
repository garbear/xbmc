/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderLutDX.h"
#include "ShaderSamplerDX.h"
#include "ShaderTextureDX.h"
#include "ShaderUtilsDX.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/IShaderPreset.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/log.h"

#include <utility>

using namespace KODI;
using namespace SHADER;

CShaderLutDX::CShaderLutDX(const std::string& id, const std::string& path) :
  IShaderLut(id, path)
{
}

CShaderLutDX::~CShaderLutDX() = default;

bool CShaderLutDX::Create(RETRO::CRenderContext &context, const ShaderLut &lut)
{
  std::unique_ptr<IShaderSampler> lutSampler(CreateLUTSampler(context, lut));
  if (!lutSampler)
  {
    CLog::Log(LOGWARNING, "%s - Couldn't create a LUT sampler for LUT %s", __FUNCTION__, lut.strId);
    return false;
  }

  std::unique_ptr<IShaderTexture> lutTexture(CreateLUTexture(lut));
  if (!lutTexture)
  {
    CLog::Log(LOGWARNING, "%s - Couldn't create a LUT texture for LUT %s", __FUNCTION__, lut.strId);
    return false;
  }

  m_sampler = std::move(lutSampler);
  m_texture = std::move(lutTexture);

  return true;
}

std::unique_ptr<IShaderSampler> CShaderLutDX::CreateLUTSampler(RETRO::CRenderContext &context, const ShaderLut &lut)
{
  CRenderSystemDX *renderingDx = static_cast<CRenderSystemDX*>(context.Rendering());

  ID3D11SamplerState* samp;
  D3D11_SAMPLER_DESC sampDesc;

  auto wrapType = CShaderUtilsDX::TranslateWrapType(lut.wrap);
  auto filterType = lut.filter ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;

  ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
  sampDesc.Filter = filterType;
  sampDesc.AddressU = wrapType;
  sampDesc.AddressV = wrapType;
  sampDesc.AddressW = wrapType;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampDesc.MinLOD = 0;
  sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

  FLOAT blackBorder[4] = { 0, 1, 0, 1 };  // TODO: turn this back to black
  memcpy(sampDesc.BorderColor, &blackBorder, 4 * sizeof(FLOAT));

  ID3D11Device* pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (FAILED(pDevice->CreateSamplerState(&sampDesc, &samp)))
  {
    CLog::Log(LOGWARNING, "%s - failed to create LUT sampler for LUT &s", __FUNCTION__, lut.path.c_str());
    return std::unique_ptr<IShaderSampler>();
  }

  // todo: take care of allocation(?)
  return std::unique_ptr<IShaderSampler>(new CShaderSamplerDX(samp));
}

std::unique_ptr<IShaderTexture> CShaderLutDX::CreateLUTexture(const ShaderLut &lut)
{
  CDXTexture* texture = static_cast<CDXTexture*>(CDXTexture::LoadFromFile(lut.path));
  if (!texture)
  {
    CLog::Log(LOGERROR, "Couldn't open LUT %s", lut.path);
    return std::unique_ptr<IShaderTexture>();
  }

  if (lut.mipmap)
    texture->SetMipmapping();

  if (texture)
    texture->LoadToGPU();

  // todo: take care of allocation(?)
  return std::unique_ptr<IShaderTexture>(new CShaderTextureCDX(texture));
}
