#pragma once
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

#include <map>
#include <algorithm>

// TODO: remove (see below)
#ifdef _WIN32
#include <DirectXMath.h>
#endif

namespace KODI
{
namespace SHADER
{
  struct float2
  {
    float2() : x(0), y(0) {}

    template<typename T>
    float2(T x_, T y_) : x(static_cast<float>(x_)), y(static_cast<float>(y_))
    {
      static_assert(std::is_arithmetic<T>::value, "Not an arithmetic type");
    }

    template<typename T>
    T Max() { return static_cast<T>(std::max(x, y)); }
    template<typename T>
    T Min() { return static_cast<T>(std::min(x, y)); }

    //TODO: move to VideoShaderUtilsDX
#ifdef _WIN32
    DirectX::XMFLOAT2 ToDXVector() const
    {
      return DirectX::XMFLOAT2(static_cast<float>(x), static_cast<float>(y));
    }
#endif

    float x;
    float y;
  };

  inline bool operator==(const float2& lhs, const float2& rhs)
  {
    return lhs.x == rhs.x && lhs.y == rhs.y;
  }

  inline bool operator!=(const float2& lhs, const float2& rhs)
  {
    return !(lhs == rhs);
  }

  class CVideoShaderUtils
  {
  public:
    /*!
     * \brief Returns smallest possible power-of-two sized texture
     */
    static float2 GetOptimalTextureSize(float2 videoSize);
  };
}
}
