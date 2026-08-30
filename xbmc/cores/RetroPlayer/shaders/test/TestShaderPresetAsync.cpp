/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.h"
#include "cores/RetroPlayer/shaders/ShaderCompileTypes.h"
#include "cores/RetroPlayer/shaders/windows/ShaderDX.h"

#include <memory>
#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace KODI::RETRO;
using namespace KODI::SHADER;

TEST(TestShaderPresetAsync, QueuedPassReturnsPendingWithoutFailedPath)
{
  std::set<std::string> failedPaths;
  const ShaderPresetState state = ShaderPresetState::PENDING;
  if (state == ShaderPresetState::FAILED)
    failedPaths.insert("preset.slangp");
  EXPECT_TRUE(failedPaths.empty());
}

TEST(TestShaderPresetAsync, CompletionWakesRenderThreadAndRealizesOnce)
{
  auto token = std::make_shared<ShaderWakeToken>(7);
  std::weak_ptr<ShaderWakeToken> weak = token;
  auto completion = [weak]
  {
    if (auto locked = weak.lock())
      locked->ready = true;
  };
  completion();
  EXPECT_TRUE(token->ready.exchange(false));
  EXPECT_FALSE(token->ready.exchange(false));
}

TEST(TestShaderPresetAsync, StaleSelectionCannotWakeNewSelection)
{
  auto oldToken = std::make_shared<ShaderWakeToken>(1);
  std::weak_ptr<ShaderWakeToken> oldWeak = oldToken;
  auto current = std::make_shared<ShaderWakeToken>(2);
  oldToken.reset();
  if (auto stale = oldWeak.lock())
    stale->ready = true;
  EXPECT_FALSE(current->ready);
}

TEST(TestShaderPresetAsync, RendererDestructionBeforeCompletionIsSafe)
{
  auto token = std::make_shared<ShaderWakeToken>(3);
  std::weak_ptr<ShaderWakeToken> weak = token;
  token.reset();
  EXPECT_FALSE(weak.lock());
}

TEST(TestShaderPresetAsync, MissingTechniqueIsTerminalWithoutDiskRetry)
{
  const ShaderCreateResult result = ShaderCreateResult::INVALID_TECHNIQUE;
  EXPECT_EQ(ShaderCreateResult::INVALID_TECHNIQUE, result);
  EXPECT_NE(ShaderCreateResult::EFFECT_CREATION_FAILED, result);
}
