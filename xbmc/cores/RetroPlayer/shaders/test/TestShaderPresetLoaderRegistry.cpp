/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/shaders/IShaderPresetLoader.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "ShaderPresetFactoryTestAccess.h"
#include "threads/Event.h"

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::SHADER;
using namespace std::chrono_literals;

namespace
{
struct LoaderCounters
{
  std::atomic_uint calls{0};
  std::atomic_uint active{0};
  std::atomic_uint maxActive{0};
  std::atomic_uint destroyed{0};
};

class FakeLoader final : public IShaderPresetLoader
{
public:
  FakeLoader(std::string source,
             std::shared_ptr<LoaderCounters> counters,
             CEvent* entered = nullptr,
             CEvent* release = nullptr,
             bool succeeds = true)
    : m_source(std::move(source)),
      m_counters(std::move(counters)),
      m_entered(entered),
      m_release(release),
      m_succeeds(succeeds)
  {
  }

  ~FakeLoader() override { ++m_counters->destroyed; }

  bool LoadPreset(std::string_view, ShaderPresetDefinition& definition) override
  {
    ++m_counters->calls;
    const unsigned int active = ++m_counters->active;
    unsigned int observed = m_counters->maxActive;
    while (active > observed &&
           !m_counters->maxActive.compare_exchange_weak(observed, active))
    {
    }
    if (m_entered)
      m_entered->Set();
    if (m_release)
      m_release->Wait();
    --m_counters->active;
    if (!m_succeeds)
      return false;
    ShaderPass pass;
    pass.sourcePath = m_source;
    definition.passes.emplace_back(std::move(pass));
    return true;
  }

private:
  std::string m_source;
  std::shared_ptr<LoaderCounters> m_counters;
  CEvent* m_entered{nullptr};
  CEvent* m_release{nullptr};
  bool m_succeeds{true};
};
} // namespace

TEST(TestShaderPresetLoaderRegistry, LoadsIntoDefinitionWithoutRendererObject)
{
  auto factory = CShaderPresetFactoryTestAccess::Create();
  auto counters = std::make_shared<LoaderCounters>();
  auto loader = std::make_shared<FakeLoader>("data-only.fx", counters);
  CShaderPresetFactoryTestAccess::Publish(*factory, loader, {".slangp", "cgp"});

  ShaderPresetDefinition definition;
  ASSERT_TRUE(factory->LoadPreset("example.slangp", definition));
  ASSERT_EQ(1u, definition.passes.size());
  EXPECT_EQ("data-only.fx", definition.passes.front().sourcePath);
  EXPECT_TRUE(factory->CanLoadPreset("example.cgp"));
}

TEST(TestShaderPresetLoaderRegistry, InFlightLoadSurvivesAddonSnapshotReplacement)
{
  auto factory = CShaderPresetFactoryTestAccess::Create();
  auto counters = std::make_shared<LoaderCounters>();
  CEvent entered;
  CEvent release{true};
  auto oldLoader = std::make_shared<FakeLoader>("old.fx", counters, &entered, &release);
  auto newLoader = std::make_shared<FakeLoader>("new.fx", counters);
  std::weak_ptr<IShaderPresetLoader> oldWeak = oldLoader;
  CShaderPresetFactoryTestAccess::Publish(*factory, oldLoader, {"slangp"});

  ShaderPresetDefinition oldDefinition;
  auto load = std::async(std::launch::async,
                         [&] { return factory->LoadPreset("one.slangp", oldDefinition); });
  ASSERT_TRUE(entered.Wait(5s));
  auto replacement = std::async(std::launch::async,
                                [&]
                                {
                                  CShaderPresetFactoryTestAccess::Replace(
                                      *factory, oldLoader, newLoader, {"slangp"});
                                });
  EXPECT_EQ(std::future_status::timeout, replacement.wait_for(100ms));
  release.Set();
  EXPECT_FALSE(load.get());
  replacement.get();
  oldLoader.reset();
  EXPECT_TRUE(oldWeak.expired());

  ShaderPresetDefinition newDefinition;
  ASSERT_TRUE(factory->LoadPreset("two.slangp", newDefinition));
  EXPECT_EQ("new.fx", newDefinition.passes.front().sourcePath);
}

TEST(TestShaderPresetLoaderRegistry, ReinstallReplacesOldAndFailedSameIdInstance)
{
  auto factory = CShaderPresetFactoryTestAccess::Create();
  auto counters = std::make_shared<LoaderCounters>();
  auto failed = std::make_shared<FakeLoader>("failed.fx", counters, nullptr, nullptr, false);
  auto recovered = std::make_shared<FakeLoader>("recovered.fx", counters);
  CShaderPresetFactoryTestAccess::Publish(*factory, failed, {"slangp"});
  ShaderPresetDefinition definition;
  EXPECT_FALSE(factory->LoadPreset("preset.slangp", definition));

  CShaderPresetFactoryTestAccess::Replace(*factory, failed, recovered, {"slangp"});
  ASSERT_TRUE(factory->LoadPreset("preset.slangp", definition));
  EXPECT_EQ("recovered.fx", definition.passes.front().sourcePath);
}

TEST(TestShaderPresetLoaderRegistry, FirstLoaderRetainsAnAlreadyRegisteredExtension)
{
  auto factory = CShaderPresetFactoryTestAccess::Create();
  auto firstCounters = std::make_shared<LoaderCounters>();
  auto secondCounters = std::make_shared<LoaderCounters>();
  auto first = std::make_shared<FakeLoader>("first.fx", firstCounters);
  auto second = std::make_shared<FakeLoader>("second.fx", secondCounters);
  CShaderPresetFactoryTestAccess::Publish(*factory, first, {"slangp"});
  CShaderPresetFactoryTestAccess::Publish(*factory, second, {"slangp"});

  ShaderPresetDefinition definition;
  ASSERT_TRUE(factory->LoadPreset("preset.slangp", definition));
  ASSERT_EQ(1u, definition.passes.size());
  EXPECT_EQ("first.fx", definition.passes.front().sourcePath);
  EXPECT_EQ(1u, firstCounters->calls);
  EXPECT_EQ(0u, secondCounters->calls);
}

TEST(TestShaderPresetLoaderRegistry, BlockedReinstallRecoversAfterOldGenerationReleases)
{
  auto factory = CShaderPresetFactoryTestAccess::Create();
  constexpr std::string_view addonId{"game.shader.blocked"};

  CShaderPresetFactoryTestAccess::BlockReinstall(*factory, std::string{addonId});
  EXPECT_TRUE(CShaderPresetFactoryTestAccess::ShouldSkipBlockedReinstall(*factory, addonId, true));
  EXPECT_TRUE(CShaderPresetFactoryTestAccess::IsReinstallBlocked(*factory, addonId));

  EXPECT_FALSE(
      CShaderPresetFactoryTestAccess::ShouldSkipBlockedReinstall(*factory, addonId, false));
  EXPECT_FALSE(CShaderPresetFactoryTestAccess::IsReinstallBlocked(*factory, addonId));
}

TEST(TestShaderPresetLoaderRegistry, ConcurrentLookupAndPublicationAreSafe)
{
  auto factory = CShaderPresetFactoryTestAccess::Create();
  auto counters = std::make_shared<LoaderCounters>();
  std::shared_ptr<IShaderPresetLoader> current =
      std::make_shared<FakeLoader>("generation-0.fx", counters);
  CShaderPresetFactoryTestAccess::Publish(*factory, current, {"slangp", "cgp"});

  std::atomic_bool stop{false};
  auto reader = std::async(std::launch::async,
                           [&]
                           {
                             while (!stop)
                             {
                               ShaderPresetDefinition definition;
                               factory->LoadPreset("preset.slangp", definition);
                               factory->CanLoadPreset("preset.cgp");
                             }
                           });
  for (unsigned int generation = 1; generation <= 25; ++generation)
  {
    auto next = std::make_shared<FakeLoader>("generation-" + std::to_string(generation) + ".fx",
                                             counters);
    CShaderPresetFactoryTestAccess::Replace(*factory, current, next, {"slangp", "cgp"});
    current = std::move(next);
  }
  stop = true;
  reader.get();
  ShaderPresetDefinition finalDefinition;
  ASSERT_TRUE(factory->LoadPreset("preset.slangp", finalDefinition));
  EXPECT_EQ("generation-25.fx", finalDefinition.passes.front().sourcePath);
  EXPECT_EQ(1u, counters->maxActive);
}
