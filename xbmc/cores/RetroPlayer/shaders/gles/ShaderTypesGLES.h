/*
 *  Copyright (C) 2019-2025 Team Kodi
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

class CShaderLutGLES;
using ShaderLutPtrGLES = std::shared_ptr<CShaderLutGLES>;
using ShaderLutVecGLES = std::vector<ShaderLutPtrGLES>;

} // namespace SHADER
} // namespace KODI
