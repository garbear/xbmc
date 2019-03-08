/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/ShaderTypes.h"

#include <d3d11.h>

namespace KODI
{
namespace SHADER
{
  class CShaderUtilsDX
  {
  public:
    static D3D11_TEXTURE_ADDRESS_MODE TranslateWrapType(WRAP_TYPE wrap);
  };

  /* todo
  operator DirectX::XMFLOAT2(const float2& f) const
  {
    return DirectX::XMFLOAT2(static_cast<float>(f.x), static_cast<float>(f.y));
  }
  */
}
}
