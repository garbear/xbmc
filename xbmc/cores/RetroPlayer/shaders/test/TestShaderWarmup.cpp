/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetFactoryTestAccess.h"
#include "ServiceBroker.h"
#include "jobs/JobManager.h"
#include "jobs/LambdaJob.h"
#include "threads/Event.h"

#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <future>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>

#include <gtest/gtest.h>

using namespace KODI::SHADER;
using namespace std::chrono_literals;

namespace
{
constexpr auto A = "C:\\catalog\\a.slangp";
constexpr auto B = "C:\\catalog\\b.slangp";
constexpr auto BAD = "C:\\catalog\\bad.slangp";
constexpr auto HIDDEN = "C:\\catalog\\hidden.slangp";

ShaderCompileKey Key(std::string_view value)
{
  ShaderCompileKey key;
  key.raw.fill(static_cast<std::uint8_t>(value.empty() ? 0 : value.front()));
  constexpr char HEX[] = "0123456789abcdef";
  for (const auto byte : key.raw)
  {
    key.hex.push_back(HEX[byte >> 4]);
    key.hex.push_back(HEX[byte & 0xf]);
  }
  return key;
}

struct Input final : IShaderCompileInput
{
  std::string value;
  bool prepareFail{false};
  bool compileFail{false};
};

struct Prepared final : IShaderPreparedUnit
{
  Prepared(std::string value_, bool fail_) : value(std::move(value_)), fail(fail_) {}
  std::string value;
  bool fail{false};
};

class WarmupCompiler final : public IShaderCompiler
{
public:
  std::string_view GetBackendId() const override { return "warmup"; }

  ShaderCompileRequest CreateRequest(const ShaderPass& pass,
                                     ShaderCompileContext context) const override
  {
    auto input = std::make_shared<Input>();
    input->value = pass.vertexSource;
    input->prepareFail = pass.fragmentSource == "prepare-fail";
    input->compileFail = pass.fragmentSource == "compile-fail";
    return {pass.sourcePath, std::move(input), std::move(context)};
  }

  ShaderPrepareResult Prepare(const IShaderCompileInput& opaque) const override
  {
    const auto& input = static_cast<const Input&>(opaque);
    if (input.prepareFail)
      return {{}, "prepare:" + input.value, {}, "prepare failed"};
    return {Key(input.value), {}, std::make_shared<Prepared>(input.value, input.compileFail), {}};
  }

  ShaderCompileResult Compile(const IShaderPreparedUnit& opaque) const override
  {
    ++compileCount;
    if (compileEntered)
      compileEntered->Set();
    if (compileRelease)
      compileRelease->Wait();
    const auto& prepared = static_cast<const Prepared&>(opaque);
    if (prepared.fail)
      return {{}, "compile failed"};
    return {{prepared.value.begin(), prepared.value.end()}, {}};
  }

  mutable std::atomic_uint compileCount{0};
  CEvent* compileEntered{nullptr};
  CEvent* compileRelease{nullptr};
};

class WarmupStore final : public IShaderArtifactStore
{
public:
  ShaderCacheLoadResult Load(const ShaderCompileKey& key) override
  {
    if (entered)
      entered->Set();
    if (release)
      release->Wait();
    std::unique_lock lock(mutex);
    const auto it = artifacts.find(key.hex);
    if (it == artifacts.end())
      return {};
    return {ShaderCacheLoadState::HIT,
            std::make_shared<const std::vector<std::uint8_t>>(it->second)};
  }

  bool Store(const ShaderCompileKey& key, std::span<const std::uint8_t> payload) override
  {
    std::unique_lock lock(mutex);
    artifacts[key.hex] = {payload.begin(), payload.end()};
    return true;
  }

  void Remove(const ShaderCompileKey& key) override
  {
    std::unique_lock lock(mutex);
    artifacts.erase(key.hex);
  }

  void Put(std::string_view value)
  {
    std::unique_lock lock(mutex);
    artifacts[Key(value).hex] = {1, 2, 3};
  }

  CEvent* entered{nullptr};
  CEvent* release{nullptr};

private:
  std::mutex mutex;
  std::map<std::string, std::vector<std::uint8_t>, std::less<>> artifacts;
};

struct WarmupManagerBlockers
{
  std::array<CEvent, 3> entered;
  CEvent release{true};
};

std::shared_ptr<WarmupManagerBlockers> SaturateWarmupWorkers()
{
  auto blockers = std::make_shared<WarmupManagerBlockers>();
  for (unsigned int index = 0; index < 3; ++index)
  {
    EXPECT_NE(0u, CServiceBroker::GetJobManager()->AddJob(
                      new CLambdaJob([blockers, index]
                                     {
                                       blockers->entered[index].Set();
                                       blockers->release.Wait();
                                     }),
                      nullptr, CJob::PRIORITY_LOW));
    EXPECT_TRUE(blockers->entered[index].Wait(5s));
  }
  return blockers;
}

ShaderPass Pass(std::string value, std::string failure = {})
{
  ShaderPass pass;
  pass.sourcePath = "C:\\shaders\\" + value + ".fx";
  pass.vertexSource = std::move(value);
  pass.fragmentSource = std::move(failure);
  pass.alias = pass.vertexSource;
  return pass;
}

class CatalogLoader final : public IShaderPresetLoader
{
public:
  bool LoadPreset(std::string_view path, ShaderPresetDefinition& definition) override
  {
    {
      std::unique_lock lock(mutex);
      ++calls[std::string{path}];
      if (throwPaths.contains(path))
        throw std::runtime_error("fake parser exception");
    }
    if (entered)
      entered->Set();
    if (release)
      release->Wait();
    std::unique_lock lock(mutex);
    const auto it = definitions.find(path);
    if (it == definitions.end())
      return false;
    definition = it->second;
    return true;
  }

  unsigned int Calls(std::string_view path)
  {
    std::unique_lock lock(mutex);
    return calls[std::string{path}];
  }

  std::mutex mutex;
  std::map<std::string, ShaderPresetDefinition, std::less<>> definitions;
  std::map<std::string, unsigned int, std::less<>> calls;
  std::set<std::string, std::less<>> throwPaths;
  CEvent* entered{nullptr};
  CEvent* release{nullptr};
};

class SummarySink
{
public:
  void Add(const ShaderWarmupSummary& summary)
  {
    std::unique_lock lock(mutex);
    summaries.emplace_back(summary);
    changed.notify_all();
  }

  ShaderWarmupSummary Wait(std::size_t count)
  {
    std::unique_lock lock(mutex);
    EXPECT_TRUE(changed.wait_for(lock, 5s, [&] { return summaries.size() >= count; }));
    return summaries.at(count - 1);
  }

  std::mutex mutex;
  std::condition_variable changed;
  std::vector<ShaderWarmupSummary> summaries;
};

class TestShaderWarmup : public testing::Test
{
protected:
  void SetUp() override
  {
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
    factory = CShaderPresetFactoryTestAccess::Create();
    loader = std::make_shared<CatalogLoader>();
    compiler = std::make_shared<WarmupCompiler>();
    CShaderPresetFactoryTestAccess::Publish(*factory, loader, {"slangp"});
    CShaderPresetFactoryTestAccess::RegisterCompiler(*factory, compiler);
    CShaderPresetFactoryTestAccess::SetSummaryCallback(
        *factory, [this](const ShaderWarmupSummary& summary) { sink.Add(summary); });
  }

  void TearDown() override
  {
    factory.reset();
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
    CServiceBroker::UnregisterJobManager();
  }

  void Set(std::string path, std::vector<ShaderPass> passes)
  {
    loader->definitions.emplace(std::move(path), ShaderPresetDefinition{std::move(passes)});
  }

  std::unique_ptr<CShaderPresetFactory> factory;
  std::shared_ptr<CatalogLoader> loader;
  std::shared_ptr<WarmupCompiler> compiler;
  SummarySink sink;
};
} // namespace

TEST_F(TestShaderWarmup, CompilesOnlyPassesFromSuppliedExposedPresetPaths)
{
  Set(A, {Pass("shared"), Pass("a")});
  Set(B, {Pass("shared"), Pass("b")});
  Set(HIDDEN, {Pass("hidden")});
  factory->WarmupPresets("warmup", {B, A, A});
  const auto summary = sink.Wait(1);
  EXPECT_EQ(2u, summary.presets);
  EXPECT_EQ(4u, summary.passes);
  EXPECT_EQ(3u, summary.unique);
  EXPECT_EQ(3u, summary.queued);
  EXPECT_EQ(0u, loader->Calls(HIDDEN));
  EXPECT_EQ(3u, compiler->compileCount);
}

TEST_F(TestShaderWarmup, ConcurrentIdenticalCatalogsCoalesce)
{
  Set(A, {Pass("a")});
  CEvent entered;
  CEvent release{true};
  loader->entered = &entered;
  loader->release = &release;
  factory->WarmupPresets("warmup", {A});
  const bool parseEntered = entered.Wait(5s);
  if (!parseEntered)
    release.Set();
  ASSERT_TRUE(parseEntered);
  factory->WarmupPresets("warmup", {A});
  release.Set();
  sink.Wait(1);
  EXPECT_EQ(1u, loader->Calls(A));
}

TEST_F(TestShaderWarmup, ReopenAfterEnumerationReparsesWhileCompileIsPending)
{
  Set(A, {Pass("slow")});
  CEvent compileEntered;
  CEvent compileRelease{true};
  compiler->compileEntered = &compileEntered;
  compiler->compileRelease = &compileRelease;
  CEvent requestEntered;
  CEvent releaseEnumeration{true};
  CShaderPresetFactoryTestAccess::SetRequestCallback(
      *factory,
      [&requestEntered, &releaseEnumeration](const std::shared_ptr<CShaderCompileHandle>&)
      {
        requestEntered.Set();
        releaseEnumeration.Wait();
      });

  factory->WarmupPresets("warmup", {A});
  const bool requestWasMade = requestEntered.Wait(5s);
  if (!requestWasMade)
  {
    releaseEnumeration.Set();
    compileRelease.Set();
  }
  ASSERT_TRUE(requestWasMade);
  factory->WarmupPresets("warmup", {A});
  EXPECT_EQ(1u, loader->Calls(A));

  releaseEnumeration.Set();
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (loader->Calls(A) < 2 && std::chrono::steady_clock::now() < deadline)
  {
    factory->WarmupPresets("warmup", {A});
    std::this_thread::yield();
  }
  const bool reopened = loader->Calls(A) == 2;
  compileRelease.Set();
  EXPECT_TRUE(reopened);
  sink.Wait(reopened ? 2 : 1);
}

TEST_F(TestShaderWarmup, LaterReopenRevalidatesAndUsesReadyEntries)
{
  Set(A, {Pass("a")});
  factory->WarmupPresets("warmup", {A});
  EXPECT_EQ(1u, sink.Wait(1).queued);
  factory->WarmupPresets("warmup", {A});
  EXPECT_EQ(1u, sink.Wait(2).memoryHits);
  EXPECT_EQ(2u, loader->Calls(A));
  EXPECT_EQ(1u, compiler->compileCount);
}

TEST_F(TestShaderWarmup, UnsupportedBackendReturnsBeforeParsing)
{
  Set(A, {Pass("a")});
  factory->WarmupPresets("unsupported", {A});
  EXPECT_EQ(0u, loader->Calls(A));
  std::unique_lock lock(sink.mutex);
  EXPECT_TRUE(sink.summaries.empty());
}

TEST_F(TestShaderWarmup, ParseFailureDoesNotAbortOtherPresets)
{
  Set(A, {Pass("a")});
  factory->WarmupPresets("warmup", {BAD, A});
  const auto summary = sink.Wait(1);
  EXPECT_EQ(2u, summary.presets);
  EXPECT_EQ(1u, summary.passes);
  EXPECT_EQ(1u, summary.unique);
}

TEST_F(TestShaderWarmup, ParserExceptionDoesNotAbortOrStickCatalog)
{
  Set(A, {Pass("a")});
  loader->throwPaths.emplace(BAD);
  factory->WarmupPresets("warmup", {BAD, A});
  EXPECT_EQ(1u, sink.Wait(1).unique);
  factory->WarmupPresets("warmup", {BAD, A});
  EXPECT_EQ(1u, sink.Wait(2).memoryHits);
  EXPECT_EQ(2u, loader->Calls(BAD));
  EXPECT_EQ(2u, loader->Calls(A));
}

TEST_F(TestShaderWarmup, SummarySeparatesMemoryDiskQueuedAndFailed)
{
  auto store = std::make_shared<WarmupStore>();
  store->Put("disk");
  CShaderPresetFactoryTestAccess::RegisterCompiler(*factory, compiler, store);
  Set(A, {Pass("disk"), Pass("ready"), Pass("compile-error", "compile-fail")});
  factory->WarmupPresets("warmup", {A});
  const auto summary = sink.Wait(1);
  EXPECT_EQ(3u, summary.unique);
  EXPECT_EQ(1u, summary.diskHits);
  EXPECT_EQ(1u, summary.queued);
  EXPECT_EQ(1u, summary.failed);
  EXPECT_EQ(summary.unique,
            summary.memoryHits + summary.diskHits + summary.queued + summary.failed);
}

TEST_F(TestShaderWarmup, CoalescedDiskFollowerCountsAsOneDiskHit)
{
  auto store = std::make_shared<WarmupStore>();
  store->Put("disk-shared");
  CEvent entered;
  CEvent release{true};
  store->entered = &entered;
  store->release = &release;
  CShaderPresetFactoryTestAccess::RegisterCompiler(*factory, compiler, store);

  auto externalPass = Pass("disk-shared");
  externalPass.sourcePath = "C:\\shared\\same.fx";
  auto owner = factory->CompileService().Request("warmup", externalPass, {});
  ASSERT_TRUE(entered.Wait(5s));
  auto followerPass = Pass("disk-shared");
  followerPass.sourcePath = externalPass.sourcePath;
  Set(A, {followerPass});
  CEvent followerRequested;
  CShaderPresetFactoryTestAccess::SetRequestCallback(
      *factory,
      [&followerRequested](const std::shared_ptr<CShaderCompileHandle>& handle)
      {
        EXPECT_EQ(ShaderRequestDisposition::COALESCED, handle->GetDisposition());
        followerRequested.Set();
      });
  factory->WarmupPresets("warmup", {A});
  CEvent ownerDone;
  owner->AddCompletionCallback([&ownerDone] { ownerDone.Set(); });
  const bool followerWasRequested = followerRequested.Wait(5s);
  release.Set();
  ASSERT_TRUE(followerWasRequested);
  EXPECT_TRUE(ownerDone.Wait(5s));
  const auto summary = sink.Wait(1);
  EXPECT_EQ(1u, summary.unique);
  EXPECT_EQ(1u, summary.diskHits);
  EXPECT_EQ(0u, summary.queued);
}

TEST_F(TestShaderWarmup, CanonicalFailuresCountOnceByStableIdentity)
{
  Set(A, {Pass("same-error", "compile-fail")});
  Set(B, {Pass("same-error", "compile-fail")});
  factory->WarmupPresets("warmup", {A, B});
  const auto summary = sink.Wait(1);
  EXPECT_EQ(1u, summary.unique);
  EXPECT_EQ(1u, summary.failed);
}

TEST_F(TestShaderWarmup, PreparationFailuresCountOnceByStableIdentity)
{
  Set(A, {Pass("same-prepare", "prepare-fail")});
  Set(B, {Pass("same-prepare", "prepare-fail")});
  factory->WarmupPresets("warmup", {A, B});
  const auto summary = sink.Wait(1);
  EXPECT_EQ(1u, summary.unique);
  EXPECT_EQ(1u, summary.failed);
}

TEST_F(TestShaderWarmup, ActiveSignatureClearsOnEveryExit)
{
  Set(A, {Pass("a")});
  factory->WarmupPresets("warmup", {A});
  sink.Wait(1);
  factory->WarmupPresets("warmup", {A});
  sink.Wait(2);
  EXPECT_EQ(2u, loader->Calls(A));
}

TEST_F(TestShaderWarmup, ParseOnlyFailureClearsActiveSignature)
{
  factory->WarmupPresets("warmup", {BAD});
  EXPECT_EQ(0u, sink.Wait(1).unique);
  factory->WarmupPresets("warmup", {BAD});
  EXPECT_EQ(0u, sink.Wait(2).unique);
  EXPECT_EQ(2u, loader->Calls(BAD));
}

TEST_F(TestShaderWarmup, RejectedEnumerationSubmissionClearsActiveSignature)
{
  Set(A, {Pass("a")});
  CServiceBroker::GetJobManager()->CancelJobs();
  factory->WarmupPresets("warmup", {A});
  CServiceBroker::GetJobManager()->Restart();
  factory->WarmupPresets("warmup", {A});
  EXPECT_EQ(1u, sink.Wait(1).unique);
  EXPECT_EQ(1u, loader->Calls(A));
}

TEST_F(TestShaderWarmup, AcceptedThenCanceledEnumerationClearsActiveSignature)
{
  Set(A, {Pass("a")});
  auto blockers = SaturateWarmupWorkers();
  factory->WarmupPresets("warmup", {A});
  auto cancel = std::async(std::launch::async,
                           [] { CServiceBroker::GetJobManager()->CancelJobs(); });
  const auto stopDeadline = std::chrono::steady_clock::now() + 5s;
  while (CServiceBroker::GetJobManager()->IsRunning() &&
         std::chrono::steady_clock::now() < stopDeadline)
    std::this_thread::yield();
  const bool managerStopped = !CServiceBroker::GetJobManager()->IsRunning();
  blockers->release.Set();
  EXPECT_TRUE(managerStopped);
  cancel.get();
  CServiceBroker::GetJobManager()->Restart();
  factory->WarmupPresets("warmup", {A});
  EXPECT_EQ(1u, sink.Wait(1).unique);
  EXPECT_EQ(1u, loader->Calls(A));
}

TEST_F(TestShaderWarmup, FactoryDestructionDuringBlockedParsingIsSafe)
{
  Set(A, {Pass("a")});
  CEvent entered;
  CEvent release{true};
  loader->entered = &entered;
  loader->release = &release;
  factory->WarmupPresets("warmup", {A});
  const bool parseEntered = entered.Wait(5s);
  if (!parseEntered)
    release.Set();
  ASSERT_TRUE(parseEntered);
  auto lifetime = CShaderPresetFactoryTestAccess::WarmupLifetime(*factory);
  auto destroy = std::async(std::launch::async, [this] { factory.reset(); });
  EXPECT_EQ(std::future_status::timeout, destroy.wait_for(100ms));
  release.Set();
  destroy.get();
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (!lifetime.expired() && std::chrono::steady_clock::now() < deadline)
    std::this_thread::yield();
  EXPECT_TRUE(lifetime.expired());
  std::unique_lock lock(sink.mutex);
  EXPECT_TRUE(sink.summaries.empty());
}

TEST_F(TestShaderWarmup, GlAndGlesReturnBeforeLoaderAccess)
{
  Set(A, {Pass("a")});
  factory->WarmupPresets("gl", {A});
  factory->WarmupPresets("gles", {A});
  EXPECT_EQ(0u, loader->Calls(A));
}
