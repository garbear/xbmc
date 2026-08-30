/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.h"
#include "cores/RetroPlayer/shaders/IShader.h"
#include "cores/RetroPlayer/shaders/IShaderTexture.h"
#include "cores/RetroPlayer/shaders/ShaderPreset.h"

#include <deque>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include <gtest/gtest.h>

using namespace KODI::RETRO;
using namespace KODI::SHADER;

namespace
{
class CTestShaderPreset : public CShaderPreset
{
public:
  explicit CTestShaderPreset(std::deque<ShaderPresetState> states) : m_states(std::move(states)) {}

  bool ReadPresetFile(const std::string& presetPath) override
  {
    ++m_readCount;
    if (m_failedReads.contains(presetPath))
      return false;
    m_passes.emplace_back();
    return !presetPath.empty();
  }

  bool HasFailed(const std::string& path) const { return HasPathFailed(path); }
  unsigned int CreateCount() const { return m_createCount; }
  unsigned int ReadCount() const { return m_readCount; }
  void FailRead(std::string path) { m_failedReads.emplace(std::move(path)); }

  void Complete()
  {
    if (m_completionCallback)
      m_completionCallback();
  }

protected:
  ShaderPresetState CreateShaders() override
  {
    ++m_createCount;
    const ShaderPresetState state = m_states.front();
    m_states.pop_front();
    if (state == ShaderPresetState::READY)
      m_pShaders.emplace_back();
    return state;
  }

  bool CreateLayouts() override { return true; }
  bool CreateBuffers() override { return true; }
  bool CreateShaderTextures() override { return true; }
  bool CreateSamplers() override { return true; }
  void RenderShader(IShader&, IShaderTexture&, IShaderTexture&) override {}

private:
  std::deque<ShaderPresetState> m_states;
  unsigned int m_createCount{0};
  unsigned int m_readCount{0};
  std::set<std::string, std::less<>> m_failedReads;
};

class CRendererShaderActivationAccess : public CRPBaseRenderer
{
public:
  static void InstallCompletionCallback(IShaderPreset& preset,
                                        const std::shared_ptr<ShaderWakeToken>& token)
  {
    CRPBaseRenderer::InstallShaderCompletionCallback(preset, token);
  }

  static void Update(IShaderPreset* preset,
                     const std::string& presetPath,
                     std::uint64_t generation,
                     const std::shared_ptr<ShaderWakeToken>& token,
                     bool& shadersNeedUpdate,
                     bool& useShaderPreset)
  {
    CRPBaseRenderer::UpdateShaderPresetActivation(preset, presetPath, generation, token,
                                                  shadersNeedUpdate, useShaderPreset);
  }
};
} // namespace

TEST(TestShaderPresetAsync, QueuedPassReturnsPendingWithoutFailedPath)
{
  CTestShaderPreset preset({ShaderPresetState::PENDING});

  EXPECT_EQ(ShaderPresetState::PENDING, preset.SetShaderPreset("preset.slangp"));
  EXPECT_FALSE(preset.HasFailed("preset.slangp"));
  EXPECT_EQ(1u, preset.ReadCount());
}

TEST(TestShaderPresetAsync, FailedPassMarksFailedPath)
{
  CTestShaderPreset preset({ShaderPresetState::FAILED});

  EXPECT_EQ(ShaderPresetState::FAILED, preset.SetShaderPreset("preset.slangp"));
  EXPECT_TRUE(preset.HasFailed("preset.slangp"));
}

TEST(TestShaderPresetAsync, ParseFailureCannotReusePassesFromPreviousPath)
{
  CTestShaderPreset preset({ShaderPresetState::PENDING, ShaderPresetState::READY});
  EXPECT_EQ(ShaderPresetState::PENDING, preset.SetShaderPreset("old.slangp"));
  preset.FailRead("bad.slangp");

  EXPECT_EQ(ShaderPresetState::FAILED, preset.SetShaderPreset("bad.slangp"));
  EXPECT_EQ(ShaderPresetState::FAILED, preset.SetShaderPreset("bad.slangp"));
  EXPECT_EQ(3u, preset.ReadCount());
  EXPECT_EQ(1u, preset.CreateCount());
}

TEST(TestShaderPresetAsync, CompletionWakesRenderThreadAndRealizesOnce)
{
  CTestShaderPreset preset({ShaderPresetState::PENDING, ShaderPresetState::READY});
  auto token = std::make_shared<ShaderWakeToken>(7);
  bool shadersNeedUpdate = true;
  bool useShaderPreset = false;

  CRendererShaderActivationAccess::Update(&preset, "preset.slangp", 7, token, shadersNeedUpdate,
                                          useShaderPreset);
  EXPECT_EQ(1u, preset.CreateCount());
  EXPECT_FALSE(useShaderPreset);

  preset.Complete();
  CRendererShaderActivationAccess::Update(&preset, "preset.slangp", 7, token, shadersNeedUpdate,
                                          useShaderPreset);
  EXPECT_EQ(2u, preset.CreateCount());
  EXPECT_EQ(1u, preset.ReadCount());
  EXPECT_TRUE(useShaderPreset);

  CRendererShaderActivationAccess::Update(&preset, "preset.slangp", 7, token, shadersNeedUpdate,
                                          useShaderPreset);
  EXPECT_EQ(2u, preset.CreateCount());
}

TEST(TestShaderPresetAsync, StaleSelectionCannotWakeNewSelection)
{
  CTestShaderPreset preset({ShaderPresetState::PENDING});
  auto oldToken = std::make_shared<ShaderWakeToken>(1);
  CRendererShaderActivationAccess::InstallCompletionCallback(preset, oldToken);

  oldToken.reset();
  auto currentToken = std::make_shared<ShaderWakeToken>(2);
  preset.Complete();

  EXPECT_FALSE(currentToken->ready);
}

TEST(TestShaderPresetAsync, RendererDestructionBeforeCompletionIsSafe)
{
  CTestShaderPreset preset({ShaderPresetState::PENDING});
  auto token = std::make_shared<ShaderWakeToken>(3);
  CRendererShaderActivationAccess::InstallCompletionCallback(preset, token);
  std::weak_ptr<ShaderWakeToken> weakToken = token;

  token.reset();
  preset.Complete();

  EXPECT_FALSE(weakToken.lock());
}
