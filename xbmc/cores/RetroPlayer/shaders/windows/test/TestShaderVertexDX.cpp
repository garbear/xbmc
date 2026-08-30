/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <windows.h>

#include "cores/RetroPlayer/shaders/windows/ShaderTypesDX.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI::SHADER;

TEST(TestShaderVertexDX, SecondTextureCoordinatesFitVertexStrideAndMatchUnitQuad)
{
  const std::array<std::array<FLOAT, 2>, 4> expectedUv1{{
      {0.0f, 0.0f},
      {1.0f, 0.0f},
      {1.0f, 1.0f},
      {0.0f, 1.0f},
  }};

  const auto vertices = CreateShaderQuad(640.0f, 480.0f);

  ASSERT_EQ(7 * sizeof(FLOAT), sizeof(CUSTOMVERTEX))
      << "TEXCOORD1 starts at byte 20 and needs two floats inside each vertex";

  for (std::size_t i = 0; i < vertices.size(); ++i)
  {
    EXPECT_FLOAT_EQ(expectedUv1[i][0], vertices[i].tu2) << "vertex " << i << " UV1.u";
    EXPECT_FLOAT_EQ(expectedUv1[i][1], vertices[i].tv2) << "vertex " << i << " UV1.v";
  }
}
