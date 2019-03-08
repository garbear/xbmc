/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

// TODO: remove (see below)
#ifdef _WIN32
#include <DirectXMath.h>
#endif

namespace KODI
{
namespace SHADER
{
  struct ShaderPass;
  using ShaderPassVec = std::vector<ShaderPass>;

  class IShaderLut;
  using ShaderLutPtr = std::shared_ptr<IShaderLut>;
  using ShaderLutVec = std::vector<ShaderLutPtr>;

  using ShaderParameterMap = std::map<std::string, float>;

  enum FILTER_TYPE
  {
    FILTER_TYPE_NONE,
    FILTER_TYPE_LINEAR,
    FILTER_TYPE_NEAREST
  };

  enum WRAP_TYPE
  {
    WRAP_TYPE_BORDER,
    WRAP_TYPE_EDGE,
    WRAP_TYPE_REPEAT,
    WRAP_TYPE_MIRRORED_REPEAT,
  };

  enum SCALE_TYPE
  {
    SCALE_TYPE_INPUT,
    SCALE_TYPE_ABSOLUTE,
    SCALE_TYPE_VIEWPORT,
  };

  struct FboScaleAxis
  {
    SCALE_TYPE type = SCALE_TYPE_INPUT;
    float scale = 1.0;
    unsigned int abs = 1;
  };

  struct FboScale
  {
    bool sRgbFramebuffer = false;
    bool floatFramebuffer = false;
    FboScaleAxis scaleX;
    FboScaleAxis scaleY;
  };

  struct ShaderLut
  {
    std::string strId;
    std::string path;
    FILTER_TYPE filter = FILTER_TYPE_NONE;
    WRAP_TYPE wrap = WRAP_TYPE_BORDER;
    bool mipmap = false;
  };

  struct ShaderParameter
  {
    std::string strId;
    std::string description;
    float current = 0.0f;
    float minimum = 0.0f;
    float initial = 0.0f;
    float maximum = 0.0f;
    float step = 0.0f;
  };

  struct ShaderPass
  {
    std::string sourcePath;
    std::string vertexSource;
    std::string fragmentSource;
    FILTER_TYPE filter = FILTER_TYPE_NONE;
    WRAP_TYPE wrap = WRAP_TYPE_BORDER;
    unsigned int frameCountMod = 0;
    FboScale fbo;
    bool mipmap = false;

    std::vector<ShaderLut> luts;
    std::vector<ShaderParameter> parameters;
  };

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

    //TODO: move to CShaderUtilsDX
#ifdef _WIN32
    DirectX::XMFLOAT2 ToDXVector() const
    {
      return DirectX::XMFLOAT2(static_cast<float>(x), static_cast<float>(y));
    }
#endif

    float x;
    float y;
  };
}
}
