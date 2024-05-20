/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderUtils.h"

using namespace KODI;
using namespace SHADER;

float2 CShaderUtils::GetOptimalTextureSize(float2 videoSize)
{
  unsigned textureWidth = 1;
  unsigned textureHeight = 1;

  // Find smallest possible power-of-two sized width that can contain the input texture
  while (true)
  {
    if (textureWidth >= videoSize.x)
      break;
    textureWidth *= 2;
  }

  // Find smallest possible power-of-two sized height that can contain the input texture
  while (true)
  {
    if (textureHeight >= videoSize.y)
      break;
    textureHeight *= 2;
  }

  return float2(textureWidth, textureHeight);
}
