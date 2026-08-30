/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "cores/RetroPlayer/shaders/ShaderCompileService.h"
#include "cores/RetroPlayer/shaders/ShaderTypes.h"
#include "jobs/JobManager.h"
#include "threads/Event.h"

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::SHADER;
using namespace std::chrono_literals;

namespace
{
ShaderCompileKey MakeKey(std::string_view value)
{
  ShaderCompileKey key;
  const char fill = value.empty() ? '0' : value.front();
  key.raw.fill(static_cast<std::uint8_t>(fill));
  constexpr char HEX[] = "0123456789abcdef";
  key.hex.reserve(64);
  for (const std::uint8_t byte : key.raw)
  {
    key.hex.push_back(HEX[byte >> 4]);
    key.hex.push_back(HEX[byte & 0x0f]);
  }
  return key;
}

struct FakeInput final : IShaderCompileInput
{
  std::string canonical;
  std::string provisional;
  bool fail{false};
};

struct FakePrepared final : IShaderPreparedUnit
{
  explicit FakePrepared(std::string value) : canonical(std::move(value)) {}
  std::string canonical;
};

class FakeCompiler final : public IShaderCompiler
{
public:
  std::string_view GetBackendId() const override { return "fake"; }

  ShaderCompileRequest CreateRequest(const ShaderPass& pass,
                                     ShaderCompileContext context) const override
  {
    auto input = std::make_shared<FakeInput>();
    input->canonical = pass.vertexSource;
    input->provisional = pass.sourcePath;
    input->fail = pass.fragmentSource == "fail";
    const std::string provisional = input->provisional;
    return {provisional, std::move(input), std::move(context)};
  }

  ShaderPrepareResult Prepare(const IShaderCompileInput& opaque) const override
  {
    ++prepareCount;
    prepareEntered.Set();
    prepareRelease.Wait();
    const auto& input = static_cast<const FakeInput&>(opaque);
    if (input.fail)
      return {{}, "failure:" + input.canonical, {}, "prepare failed"};
    return {MakeKey(input.canonical), {}, std::make_shared<FakePrepared>(input.canonical), {}};
  }

  ShaderCompileResult Compile(const IShaderPreparedUnit& opaque) const override
  {
    ++compileCount;
    compileEntered.Set();
    compileRelease.Wait();
    const auto& prepared = static_cast<const FakePrepared&>(opaque);
    compileFinished.Set();
    return {{prepared.canonical.begin(), prepared.canonical.end()}, {}};
  }

  mutable std::atomic_uint prepareCount{0};
  mutable std::atomic_uint compileCount{0};
  mutable CEvent prepareEntered{true};
  mutable CEvent compileEntered{true};
  mutable CEvent compileFinished{true};
  mutable CEvent prepareRelease{true, true};
  mutable CEvent compileRelease{true, true};
};

class FakeStore final : public IShaderArtifactStore
{
public:
  ShaderCacheLoadResult Load(const ShaderCompileKey& key) override
  {
    std::unique_lock lock(mutex);
    if (corrupt)
    {
      corrupt = false;
      return {ShaderCacheLoadState::CORRUPT, {}};
    }
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
    ++storeCount;
    return true;
  }

  void Remove(const ShaderCompileKey& key) override
  {
    std::unique_lock lock(mutex);
    artifacts.erase(key.hex);
    ++removeCount;
  }

  void Put(const ShaderCompileKey& key, std::vector<std::uint8_t> bytes)
  {
    std::unique_lock lock(mutex);
    artifacts[key.hex] = std::move(bytes);
  }

  std::mutex mutex;
  std::map<std::string, std::vector<std::uint8_t>, std::less<>> artifacts;
  bool corrupt{false};
  std::atomic_uint storeCount{0};
  std::atomic_uint removeCount{0};
};

ShaderPass Pass(std::string provisional, std::string canonical, bool fail = false)
{
  ShaderPass pass;
  pass.sourcePath = std::move(provisional);
  pass.vertexSource = std::move(canonical);
  pass.fragmentSource = fail ? "fail" : "";
  pass.alias = "pass";
  return pass;
}

void ExpectTerminal(const std::shared_ptr<CShaderCompileHandle>& handle)
{
  auto complete = std::make_shared<CEvent>();
  handle->AddCompletionCallback([complete] { complete->Set(); });
  if (handle->GetState() != ShaderCompileState::READY &&
      handle->GetState() != ShaderCompileState::FAILED)
    ASSERT_TRUE(complete->Wait(5s));
}

class TestShaderCompileService : public testing::Test
{
protected:
  void SetUp() override
  {
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
    compiler = std::make_shared<FakeCompiler>();
    store = std::make_shared<FakeStore>();
    service = std::make_unique<CShaderCompileService>();
    service->RegisterCompiler(compiler, store);
  }

  void TearDown() override
  {
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
    service.reset();
    CServiceBroker::UnregisterJobManager();
  }

  std::unique_ptr<CShaderCompileService> service;
  std::shared_ptr<FakeCompiler> compiler;
  std::shared_ptr<FakeStore> store;
};
} // namespace

TEST_F(TestShaderCompileService, IdenticalAndConcurrentRequestsCompileOnce)
{
  // Identical in-flight requests attach to one preparation and one compile.
  compiler->compileRelease.Reset();
  const ShaderPass pass = Pass("same", "a");
  auto first = service->Request("fake", pass, {});
  auto second = service->Request("fake", pass, {});
  ASSERT_TRUE(compiler->compileEntered.Wait(5s));
  EXPECT_EQ(ShaderCompileState::COMPILING, first->GetState());
  EXPECT_EQ(ShaderCompileState::COMPILING, second->GetState());
  EXPECT_EQ(ShaderRequestDisposition::QUEUED, first->GetDisposition());
  EXPECT_EQ(ShaderRequestDisposition::MEMORY_HIT, second->GetDisposition());
  compiler->compileRelease.Set();
  ExpectTerminal(first);
  ExpectTerminal(second);
  EXPECT_EQ(1u, compiler->prepareCount);
  EXPECT_EQ(1u, compiler->compileCount);
}

TEST_F(TestShaderCompileService, DifferentProvisionalRequestsConvergeOnOneCanonicalCompile)
{
  // Distinct provisional identities converge after preparation on the same canonical key.
  auto first = service->Request("fake", Pass("path-a", "b"), {});
  auto second = service->Request("fake", Pass("path-b", "b"), {});
  ExpectTerminal(first);
  ExpectTerminal(second);
  EXPECT_EQ(1u, compiler->compileCount);
}

TEST_F(TestShaderCompileService, SharedPassAcrossPresetContextsCompilesOnce)
{
  // Diagnostic preset context is excluded from canonical compilation identity.
  auto first = service->Request("fake", Pass("shared", "c"), {.presetPath = "one"});
  auto second = service->Request("fake", Pass("shared", "c"), {.presetPath = "two"});
  ExpectTerminal(first);
  ExpectTerminal(second);
  EXPECT_EQ(1u, compiler->compileCount);
}

TEST_F(TestShaderCompileService, PendingAndFailedRemainDistinct)
{
  // A failed request must not collapse an unrelated in-flight request into failure.
  compiler->compileRelease.Reset();
  auto pending = service->Request("fake", Pass("pending", "d"), {});
  ASSERT_TRUE(compiler->compileEntered.Wait(5s));
  auto failed = service->Request("fake", Pass("failed", "e", true), {});
  ExpectTerminal(failed);
  EXPECT_EQ(ShaderCompileState::COMPILING, pending->GetState());
  EXPECT_EQ(ShaderCompileState::FAILED, failed->GetState());
  compiler->compileRelease.Set();
  ExpectTerminal(pending);
}

TEST_F(TestShaderCompileService, UnchangedFailureIsNotRequeuedOrRelogged)
{
  // A stable preparation failure is memoized and never reaches the compiler.
  auto first = service->Request("fake", Pass("failure-one", "f", true), {});
  ExpectTerminal(first);
  auto second = service->Request("fake", Pass("failure-two", "f", true), {});
  ExpectTerminal(second);
  EXPECT_EQ(ShaderCompileState::FAILED, second->GetState());
  EXPECT_EQ(0u, compiler->compileCount);
  EXPECT_EQ(first->GetError(), second->GetError());
}

TEST_F(TestShaderCompileService, ValidPersistentHitDoesNotInvokeCompiler)
{
  // A validated store hit becomes an immutable disk artifact without compiling.
  store->Put(MakeKey("g"), {1, 2, 3});
  auto handle = service->Request("fake", Pass("disk", "g"), {});
  ExpectTerminal(handle);
  ASSERT_NE(nullptr, handle->GetArtifact());
  EXPECT_EQ(ShaderArtifactOrigin::DISK, handle->GetArtifact()->origin);
  EXPECT_EQ(ShaderRequestDisposition::DISK_HIT, handle->GetDisposition());
  EXPECT_EQ(0u, compiler->compileCount);
}

TEST_F(TestShaderCompileService, CorruptPersistentEntryCompilesAndRepublishes)
{
  // A corrupt envelope is treated as a miss and replaced by compiled bytes.
  store->corrupt = true;
  auto handle = service->Request("fake", Pass("corrupt", "h"), {});
  ExpectTerminal(handle);
  EXPECT_EQ(ShaderArtifactOrigin::COMPILED, handle->GetArtifact()->origin);
  EXPECT_EQ(1u, compiler->compileCount);
  EXPECT_EQ(1u, store->storeCount);
}

TEST_F(TestShaderCompileService, DiskArtifactRejectionRecompilesExactlyOnce)
{
  // Only the first rejection of a disk artifact advances generation and recompiles.
  store->Put(MakeKey("i"), {9});
  auto handle = service->Request("fake", Pass("retry", "i"), {});
  ExpectTerminal(handle);
  const std::uint64_t generation = handle->GetGeneration();
  ASSERT_TRUE(service->RejectDiskArtifact(handle, generation));
  ExpectTerminal(handle);
  EXPECT_EQ(generation + 1, handle->GetGeneration());
  EXPECT_EQ(1u, compiler->compileCount);
  EXPECT_FALSE(service->RejectDiskArtifact(handle, handle->GetGeneration()));
  EXPECT_EQ(1u, compiler->compileCount);
  EXPECT_EQ(1u, store->removeCount);
}

TEST_F(TestShaderCompileService, GroupCompletesOncePerGeneration)
{
  // Group completion fires once initially and once after a member is rearmed.
  store->Put(MakeKey("j"), {1});
  std::atomic_uint completionCount{0};
  CEvent completed;
  auto group = service->RequestGroup("fake", {Pass("group", "j")}, "preset",
                                     [&]
                                     {
                                       ++completionCount;
                                       completed.Set();
                                     });
  ASSERT_TRUE(completed.Wait(5s));
  ASSERT_EQ(ShaderPresetState::READY, group->GetState());
  auto handle = group->GetHandles().front();
  ASSERT_TRUE(service->RejectDiskArtifact(handle, handle->GetGeneration()));
  ASSERT_TRUE(completed.Wait(5s));
  EXPECT_EQ(2u, completionCount);
  EXPECT_EQ(ShaderPresetState::READY, group->GetState());
}

TEST_F(TestShaderCompileService, DestructionWithQueuedAndRunningJobsIsSafe)
{
  // Jobs capture retained state only, so destroying the service cannot create a use-after-free.
  auto localCompiler = std::make_shared<FakeCompiler>();
  localCompiler->prepareRelease.Reset();
  auto localService = std::make_unique<CShaderCompileService>();
  localService->RegisterCompiler(localCompiler);
  localService->Request("fake", Pass("one", "k"), {});
  localService->Request("fake", Pass("two", "l"), {});
  localService->Request("fake", Pass("three", "m"), {});
  ASSERT_TRUE(localCompiler->prepareEntered.Wait(5s));
  localService.reset();
  localCompiler->prepareRelease.Set();
  SUCCEED();
}
