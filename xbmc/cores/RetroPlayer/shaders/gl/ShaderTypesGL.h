/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

namespace KODI
{
namespace SHADER
{

class CShaderLutGL;
using ShaderLutPtrGL = std::shared_ptr<CShaderLutGL>;
using ShaderLutVecGL = std::vector<ShaderLutPtrGL>;

}
}
