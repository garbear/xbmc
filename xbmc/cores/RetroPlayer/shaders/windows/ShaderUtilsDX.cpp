/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderUtilsDX.h"

using namespace KODI;
using namespace SHADER;

D3D11_TEXTURE_ADDRESS_MODE CShaderUtilsDX::TranslateWrapType(WRAP_TYPE wrap)
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
