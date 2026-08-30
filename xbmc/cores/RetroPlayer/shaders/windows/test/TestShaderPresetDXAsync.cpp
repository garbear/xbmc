/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cores/RetroPlayer/shaders/windows/ShaderDX.h"

#include <gtest/gtest.h>

using namespace KODI::SHADER;

TEST(TestShaderPresetAsync, MissingTechniqueIsTerminalWithoutDiskRetry)
{
  EXPECT_FALSE(
      ShouldRetryDiskArtifact(ShaderCreateResult::INVALID_TECHNIQUE, ShaderArtifactOrigin::DISK));
  EXPECT_TRUE(ShouldRetryDiskArtifact(ShaderCreateResult::EFFECT_CREATION_FAILED,
                                      ShaderArtifactOrigin::DISK));
  EXPECT_FALSE(ShouldRetryDiskArtifact(ShaderCreateResult::EFFECT_CREATION_FAILED,
                                       ShaderArtifactOrigin::COMPILED));
}
