/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <cstddef>
#include <minwindef.h>

namespace KODI::SHADER
{
struct CUSTOMVERTEX
{
  FLOAT x;
  FLOAT y;
  FLOAT z;

  FLOAT tu;
  FLOAT tv;

  FLOAT tu2;
  FLOAT tv2;
};

static_assert(offsetof(CUSTOMVERTEX, tu2) == 5 * sizeof(FLOAT));
static_assert(sizeof(CUSTOMVERTEX) == 7 * sizeof(FLOAT));

std::array<CUSTOMVERTEX, 4> CreateShaderQuad(float width, float height);
} // namespace KODI::SHADER
