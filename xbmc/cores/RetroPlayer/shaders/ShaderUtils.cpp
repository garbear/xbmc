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
  unsigned sizeMax = videoSize.Max<unsigned>();
  unsigned size = 1;

  // Find smallest possible power-of-two size that can contain input texture
  while (true)
  {
    if (size >= sizeMax)
      break;
    size *= 2;
  }
  return float2(size, size);
}
