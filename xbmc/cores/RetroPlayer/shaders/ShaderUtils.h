/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTypes.h"

namespace KODI
{
namespace SHADER
{
  inline bool operator==(const float2& lhs, const float2& rhs)
  {
    return lhs.x == rhs.x && lhs.y == rhs.y;
  }

  inline bool operator!=(const float2& lhs, const float2& rhs)
  {
    return !(lhs == rhs);
  }

  class CShaderUtils
  {
  public:
    /*!
     * \brief Returns smallest possible power-of-two sized texture
     */
    static float2 GetOptimalTextureSize(float2 videoSize);
  };
}
}
