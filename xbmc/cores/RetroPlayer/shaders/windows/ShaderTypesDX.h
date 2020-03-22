/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <minwindef.h>
#include <memory>
#include <vector>

namespace KODI
{
namespace SHADER
{

class CShaderLutDX;
using ShaderLutPtrDX = std::shared_ptr<CShaderLutDX>;
using ShaderLutVecDX = std::vector<ShaderLutPtrDX>;

struct CUSTOMVERTEX
{
  FLOAT x;
  FLOAT y;
  FLOAT z;

  FLOAT tu;
  FLOAT tv;
};

}
}
