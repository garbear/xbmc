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

#include "VideoShaderTextureDX.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/VideoShaders/IVideoShaderPreset.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/log.h"

#include "d3d11.h"

#include <regex>

using namespace KODI;
using namespace SHADER;

IShaderSampler* SHADER::CreateLUTSampler(RETRO::CRenderContext &context, const VideoShaderLut &lut)
{
  CRenderSystemDX *renderingDx = static_cast<CRenderSystemDX*>(context.Rendering());

  ID3D11SamplerState* samp;
  D3D11_SAMPLER_DESC sampDesc;

  auto wrapType = TranslateWrapType(lut.wrap);
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
    return nullptr;
  }

  // todo: take care of allocation(?)
  return new CShaderSamplerDX(samp);
}

IShaderTexture* SHADER::CreateLUTexture(const VideoShaderLut &lut)
{
  CDXTexture* texture = static_cast<CDXTexture*>(CDXTexture::LoadFromFile(lut.path));
  if (!texture)
  {
    CLog::Log(LOGERROR, "Couldn't open LUT %s", lut.path);
    return nullptr;
  }
  if (lut.mipmap)
    texture->SetMipmapping();

  if (texture)
    texture->LoadToGPU();
  // todo: take care of allocation(?)
  return new CShaderTextureCDX(texture);
}

D3D11_TEXTURE_ADDRESS_MODE SHADER::TranslateWrapType(WRAP_TYPE wrap)
{
  D3D11_TEXTURE_ADDRESS_MODE dxWrap;
  switch(wrap)
  {
  case WRAP_TYPE_EDGE:
    dxWrap = D3D11_TEXTURE_ADDRESS_CLAMP;
    break;
  case WRAP_TYPE_REPEAT:
    dxWrap = D3D11_TEXTURE_ADDRESS_WRAP;
    break;
  case WRAP_TYPE_MIRRORED_REPEAT:
    dxWrap = D3D11_TEXTURE_ADDRESS_MIRROR;
    break;
  case WRAP_TYPE_BORDER:
  default:
    dxWrap = D3D11_TEXTURE_ADDRESS_BORDER;
  }
  return dxWrap;
}
