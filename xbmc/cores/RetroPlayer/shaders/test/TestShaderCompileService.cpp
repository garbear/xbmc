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
#include "jobs/LambdaJob.h"
#include "threads/Event.h"

#include <atomic>
#include <array>
#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::SHADER;
using namespace std::chrono_literals;

namespace KODI::SHADER
{
class CShaderCompileServiceTestAccess
{
public:
  static std::unique_ptr<CShaderCompileService> Create(std::function<void()> beforeCanonicalAttach)
  {
    return std::unique_ptr<CShaderCompileService>(
        new CShaderCompileService(std::move(beforeCanonicalAttach)));
  }

  static void SetGroupBeforeCallbackMark(CShaderCompileService& service,
                                         const std::shared_ptr<CShaderCompileGroup>& group,
                                         std::function<void()> callback)
  {
    service.SetGroupBeforeCallbackMark(group, std::move(callback));
  }
};
} // namespace KODI::SHADER

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
  bool prepareFail{false};
  bool compileFail{false};
};

struct FakePrepared final : IShaderPreparedUnit
{
  FakePrepared(std::string value, bool fail)
    : canonical(std::move(value)), compileFail(fail)
  {
  }
  std::string canonical;
  bool compileFail{false};
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
    input->prepareFail = pass.fragmentSource == "prepare-fail";
    input->compileFail = pass.fragmentSource == "compile-fail";
    const std::string provisional = input->provisional;
    return {provisional, std::move(input), std::move(context)};
  }

  ShaderPrepareResult Prepare(const IShaderCompileInput& opaque) const override
  {
    ++prepareCount;
    prepareEntered.Set();
    prepareRelease.Wait();
    const auto& input = static_cast<const FakeInput&>(opaque);
    if (input.prepareFail)
      return {{}, "failure:" + input.canonical, {}, "prepare failed"};
    return {MakeKey(input.canonical), {},
            std::make_shared<FakePrepared>(input.canonical, input.compileFail), {}};
  }

  ShaderCompileResult Compile(const IShaderPreparedUnit& opaque) const override
  {
    ++compileCount;
    compileEntered.Set();
    compileRelease.Wait();
    if (throwOnCompile)
      throw std::runtime_error("compiler exception");
    const auto& prepared = static_cast<const FakePrepared&>(opaque);
    compileFinished.Set();
    if (prepared.compileFail)
      return {{}, "compile failed"};
    return {{prepared.canonical.begin(), prepared.canonical.end()}, {}};
  }

  mutable std::atomic_uint prepareCount{0};
  mutable std::atomic_uint compileCount{0};
  mutable CEvent prepareEntered{true};
  mutable CEvent compileEntered{true};
  mutable CEvent compileFinished{true};
  mutable CEvent prepareRelease{true, true};
  mutable CEvent compileRelease{true, true};
  bool throwOnCompile{false};
};

class FakeStore final : public IShaderArtifactStore
{
public:
  ShaderCacheLoadResult Load(const ShaderCompileKey& key) override
  {
    std::unique_lock lock(mutex);
    if (throwOnLoad)
      throw std::runtime_error("store exception");
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
  bool throwOnLoad{false};
  std::atomic_uint storeCount{0};
  std::atomic_uint removeCount{0};
};

ShaderPass Pass(std::string provisional,
                std::string canonical,
                std::string failure = {})
{
  ShaderPass pass;
  pass.sourcePath = std::move(provisional);
  pass.vertexSource = std::move(canonical);
  pass.fragmentSource = std::move(failure);
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

struct ManagerBlockers
{
  std::array<CEvent, 3> entered;
  CEvent release{true};
};

std::shared_ptr<ManagerBlockers> SaturateLowPriorityWorkers()
{
  auto blockers = std::make_shared<ManagerBlockers>();
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
  const bool compileEntered = compiler->compileEntered.Wait(5s);
  if (!compileEntered)
    compiler->compileRelease.Set();
  ASSERT_TRUE(compileEntered);
  EXPECT_EQ(ShaderCompileState::COMPILING, first->GetState());
  EXPECT_EQ(ShaderCompileState::COMPILING, second->GetState());
  EXPECT_EQ(ShaderRequestDisposition::QUEUED, first->GetDisposition());
  EXPECT_EQ(ShaderRequestDisposition::COALESCED, second->GetDisposition());
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
  ASSERT_TRUE(first->GetTerminal());
  ASSERT_TRUE(second->GetTerminal());
  EXPECT_EQ(first->GetTerminal()->identity, second->GetTerminal()->identity);
}

TEST_F(TestShaderCompileService, SameProvisionalFollowersMigrateOnCanonicalConvergence)
{
  compiler->compileRelease.Reset();
  auto winner = service->Request("fake", Pass("winner-path", "shared-canonical"), {});
  const bool compileEntered = compiler->compileEntered.Wait(5s);
  if (!compileEntered)
    compiler->compileRelease.Set();
  ASSERT_TRUE(compileEntered);

  compiler->prepareEntered.Reset();
  compiler->prepareRelease.Reset();
  auto owner = service->Request("fake", Pass("shared-path", "shared-canonical"), {});
  const bool prepareEntered = compiler->prepareEntered.Wait(5s);
  if (!prepareEntered)
  {
    compiler->prepareRelease.Set();
    compiler->compileRelease.Set();
  }
  ASSERT_TRUE(prepareEntered);
  auto follower = service->Request("fake", Pass("shared-path", "shared-canonical"), {});
  compiler->prepareRelease.Set();
  compiler->compileRelease.Set();

  ExpectTerminal(winner);
  ExpectTerminal(owner);
  ExpectTerminal(follower);
  EXPECT_EQ(ShaderCompileState::READY, owner->GetState());
  EXPECT_EQ(ShaderCompileState::READY, follower->GetState());
  EXPECT_EQ(owner->GetTerminal()->identity, follower->GetTerminal()->identity);
  EXPECT_EQ(1u, compiler->compileCount);
}

TEST_F(TestShaderCompileService, SameProvisionalFollowersMigrateOnFailureConvergence)
{
  auto winner = service->Request(
      "fake", Pass("failure-winner", "shared-failure", "prepare-fail"), {});
  ExpectTerminal(winner);

  compiler->prepareEntered.Reset();
  compiler->prepareRelease.Reset();
  auto owner = service->Request(
      "fake", Pass("failure-path", "shared-failure", "prepare-fail"), {});
  const bool prepareEntered = compiler->prepareEntered.Wait(5s);
  if (!prepareEntered)
    compiler->prepareRelease.Set();
  ASSERT_TRUE(prepareEntered);
  auto follower = service->Request(
      "fake", Pass("failure-path", "shared-failure", "prepare-fail"), {});
  compiler->prepareRelease.Set();

  ExpectTerminal(owner);
  ExpectTerminal(follower);
  EXPECT_EQ(ShaderCompileState::FAILED, owner->GetState());
  EXPECT_EQ(ShaderCompileState::FAILED, follower->GetState());
  EXPECT_EQ(owner->GetTerminal()->identity, follower->GetTerminal()->identity);
}

TEST_F(TestShaderCompileService, CanonicalCompileFailuresExposeOneStableIdentity)
{
  auto first = service->Request(
      "fake", Pass("compile-one", "compile-failure", "compile-fail"),
      {.presetPath = "one.slangp", .passIndex = 1, .passAlias = "FIRST",
       .shaderPath = "first.slang"});
  auto second = service->Request(
      "fake", Pass("compile-two", "compile-failure", "compile-fail"),
      {.presetPath = "two.slangp", .passIndex = 2, .passAlias = "SECOND",
       .shaderPath = "second.slang"});
  ExpectTerminal(first);
  ExpectTerminal(second);
  EXPECT_EQ(ShaderCompileState::FAILED, first->GetState());
  ASSERT_TRUE(first->GetTerminal());
  ASSERT_TRUE(second->GetTerminal());
  EXPECT_EQ(first->GetTerminal()->identity, second->GetTerminal()->identity);
  EXPECT_EQ(ShaderCompileIdentityKind::CANONICAL, first->GetTerminal()->identity.kind);
  EXPECT_FALSE(first->GetTerminal()->identity.value.empty());
  EXPECT_EQ(1u, compiler->compileCount);
}

TEST_F(TestShaderCompileService, PreparationFailuresExposeOneStableIdentity)
{
  auto first = service->Request(
      "fake", Pass("prepare-one", "prepare-failure", "prepare-fail"),
      {.presetPath = "one.slangp", .passIndex = 1, .passAlias = "FIRST",
       .shaderPath = "first.slang"});
  auto second = service->Request(
      "fake", Pass("prepare-two", "prepare-failure", "prepare-fail"),
      {.presetPath = "two.slangp", .passIndex = 2, .passAlias = "SECOND",
       .shaderPath = "second.slang"});
  ExpectTerminal(first);
  ExpectTerminal(second);
  EXPECT_EQ(ShaderCompileState::FAILED, first->GetState());
  ASSERT_TRUE(first->GetTerminal());
  ASSERT_TRUE(second->GetTerminal());
  EXPECT_EQ(first->GetTerminal()->identity, second->GetTerminal()->identity);
  EXPECT_EQ(ShaderCompileIdentityKind::PREPARATION_FAILURE,
            first->GetTerminal()->identity.kind);
  EXPECT_FALSE(first->GetTerminal()->identity.value.empty());
  EXPECT_EQ(0u, compiler->compileCount);
}

TEST_F(TestShaderCompileService, TerminalSnapshotCannotMissConvergingRequest)
{
  // The canonical listener insertion and terminal-state observation must be one atomic operation.
  CEvent attachEntered;
  CEvent releaseAttach{true};
  service = CShaderCompileServiceTestAccess::Create(
      [&]
      {
        attachEntered.Set();
        releaseAttach.Wait();
      });
  service->RegisterCompiler(compiler, store);

  compiler->compileRelease.Reset();
  auto first = service->Request("fake", Pass("winner", "race"), {});
  const bool compileEntered = compiler->compileEntered.Wait(5s);
  if (!compileEntered)
    compiler->compileRelease.Set();
  ASSERT_TRUE(compileEntered);

  CEvent firstCompleted;
  first->AddCompletionCallback([&] { firstCompleted.Set(); });

  // Install the converger callback while preparation is blocked, before the worker can take the
  // request and canonical-entry locks used by the attach barrier.
  compiler->prepareEntered.Reset();
  compiler->prepareRelease.Reset();
  auto second = service->Request("fake", Pass("converger", "race"), {});
  const bool prepareEntered = compiler->prepareEntered.Wait(5s);
  if (!prepareEntered)
  {
    compiler->prepareRelease.Set();
    compiler->compileRelease.Set();
  }
  ASSERT_TRUE(prepareEntered);
  CEvent secondCompleted;
  second->AddCompletionCallback([&] { secondCompleted.Set(); });
  compiler->prepareRelease.Set();
  const bool didAttach = attachEntered.Wait(5s);
  if (!didAttach)
  {
    releaseAttach.Set();
    compiler->compileRelease.Set();
  }
  ASSERT_TRUE(didAttach);

  compiler->compileRelease.Set();
  const bool compileFinished = compiler->compileFinished.Wait(5s);
  if (!compileFinished)
    releaseAttach.Set();
  ASSERT_TRUE(compileFinished);
  const bool winnerCompletedBeforeAttach = firstCompleted.Wait(250ms);
  releaseAttach.Set();

  EXPECT_FALSE(winnerCompletedBeforeAttach);
  if (!winnerCompletedBeforeAttach)
    EXPECT_TRUE(firstCompleted.Wait(5s));
  EXPECT_TRUE(secondCompleted.Wait(5s));
  EXPECT_EQ(ShaderCompileState::READY, second->GetState());
}

TEST_F(TestShaderCompileService, ThrowingCompletionCallbackDoesNotSkipLaterListeners)
{
  compiler->compileRelease.Reset();
  auto handle = service->Request("fake", Pass("throwing-callback", "callback"), {});
  const bool compileEntered = compiler->compileEntered.Wait(5s);
  if (!compileEntered)
    compiler->compileRelease.Set();
  ASSERT_TRUE(compileEntered);

  CEvent laterListener;
  handle->AddCompletionCallback([] { throw std::runtime_error("callback failed"); });
  handle->AddCompletionCallback([&laterListener] { laterListener.Set(); });
  compiler->compileRelease.Set();

  EXPECT_TRUE(laterListener.Wait(5s));
  EXPECT_EQ(ShaderCompileState::READY, handle->GetState());
}

TEST_F(TestShaderCompileService, TerminalCallbackAddedDuringNotificationRunsOnce)
{
  compiler->compileRelease.Reset();
  auto first = service->Request("fake", Pass("callback-race", "callback-race"), {});
  auto second = service->Request("fake", Pass("callback-race", "callback-race"), {});
  const bool compileEntered = compiler->compileEntered.Wait(5s);
  if (!compileEntered)
    compiler->compileRelease.Set();
  ASSERT_TRUE(compileEntered);

  CEvent firstCallbackEntered;
  CEvent releaseFirstCallback{true};
  CEvent secondNotificationFinished;
  first->AddCompletionCallback(
      [&firstCallbackEntered, &releaseFirstCallback]
      {
        firstCallbackEntered.Set();
        releaseFirstCallback.Wait();
      });
  second->AddCompletionCallback([&secondNotificationFinished] { secondNotificationFinished.Set(); });
  compiler->compileRelease.Set();
  const bool notificationStarted = firstCallbackEntered.Wait(5s);
  if (!notificationStarted)
    releaseFirstCallback.Set();
  ASSERT_TRUE(notificationStarted);

  std::atomic_uint secondCount{0};
  second->AddCompletionCallback([&secondCount] { ++secondCount; });
  releaseFirstCallback.Set();
  EXPECT_TRUE(secondNotificationFinished.Wait(5s));
  EXPECT_EQ(1u, secondCount);
}

TEST_F(TestShaderCompileService, ThrowingTerminalCallbackDoesNotEscapeCaller)
{
  auto handle = service->Request("fake", Pass("terminal-callback", "terminal-callback"), {});
  ExpectTerminal(handle);

  EXPECT_NO_THROW(
      handle->AddCompletionCallback([] { throw std::runtime_error("callback failed"); }));
}

TEST_F(TestShaderCompileService, CompilerAndStoreExceptionsBecomeTerminalFailures)
{
  compiler->throwOnCompile = true;
  auto compileFailure = service->Request("fake", Pass("throwing-compile", "throwing-compile"), {});
  ExpectTerminal(compileFailure);
  EXPECT_EQ(ShaderCompileState::FAILED, compileFailure->GetState());

  compiler->throwOnCompile = false;
  store->throwOnLoad = true;
  auto storeFailure = service->Request("fake", Pass("throwing-store", "throwing-store"), {});
  ExpectTerminal(storeFailure);
  EXPECT_EQ(ShaderCompileState::FAILED, storeFailure->GetState());
}

TEST_F(TestShaderCompileService, ThrowingGroupCallbackDoesNotSkipLaterListeners)
{
  compiler->compileRelease.Reset();
  auto group = service->RequestGroup(
      "fake", {Pass("throwing-group", "throwing-group")}, "preset",
      [] { throw std::runtime_error("group callback failed"); });
  const bool compileEntered = compiler->compileEntered.Wait(5s);
  if (!compileEntered)
    compiler->compileRelease.Set();
  ASSERT_TRUE(compileEntered);

  CEvent laterListener;
  group->AddCompletionCallback([&laterListener] { laterListener.Set(); });
  compiler->compileRelease.Set();

  EXPECT_TRUE(laterListener.Wait(5s));
  EXPECT_EQ(ShaderPresetState::READY, group->GetState());
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
  auto failed = service->Request("fake", Pass("failed", "e", "prepare-fail"), {});
  ExpectTerminal(failed);
  EXPECT_EQ(ShaderCompileState::COMPILING, pending->GetState());
  EXPECT_EQ(ShaderCompileState::FAILED, failed->GetState());
  compiler->compileRelease.Set();
  ExpectTerminal(pending);
}

TEST_F(TestShaderCompileService, UnchangedFailureIsNotRequeuedOrRelogged)
{
  // A stable preparation failure is memoized and never reaches the compiler.
  auto first = service->Request("fake", Pass("failure-one", "f", "prepare-fail"), {});
  ExpectTerminal(first);
  auto second = service->Request("fake", Pass("failure-two", "f", "prepare-fail"), {});
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

TEST_F(TestShaderCompileService, LateGroupCallbackRunsForCurrentGeneration)
{
  auto group = service->RequestGroup("fake", {Pass("late-group", "late-group")}, "preset", {});
  ExpectTerminal(group->GetHandles().front());
  CEvent completed;

  group->AddCompletionCallback([&completed] { completed.Set(); });

  EXPECT_TRUE(completed.Wait(5s));
}

TEST_F(TestShaderCompileService, GroupCallbackAddedDuringNotificationIsNotMissed)
{
  compiler->compileRelease.Reset();
  CEvent firstCallbackEntered;
  CEvent releaseFirstCallback{true};
  auto group = service->RequestGroup(
      "fake", {Pass("group-race", "group-race")}, "preset",
      [&firstCallbackEntered, &releaseFirstCallback]
      {
        firstCallbackEntered.Set();
        releaseFirstCallback.Wait();
      });
  const bool compileEntered = compiler->compileEntered.Wait(5s);
  if (!compileEntered)
  {
    compiler->compileRelease.Set();
    releaseFirstCallback.Set();
  }
  ASSERT_TRUE(compileEntered);
  compiler->compileRelease.Set();
  const bool notificationStarted = firstCallbackEntered.Wait(5s);
  if (!notificationStarted)
    releaseFirstCallback.Set();
  ASSERT_TRUE(notificationStarted);

  CEvent secondCallback;
  group->AddCompletionCallback([&secondCallback] { secondCallback.Set(); });
  const bool secondDelivered = secondCallback.Wait(250ms);
  releaseFirstCallback.Set();
  EXPECT_TRUE(secondDelivered);
}

TEST_F(TestShaderCompileService, EmptyGroupCompletes)
{
  CEvent completed;
  auto group = service->RequestGroup("fake", {}, "preset", [&completed] { completed.Set(); });

  EXPECT_EQ(ShaderPresetState::READY, group->GetState());
  EXPECT_TRUE(completed.Wait(5s));
}

TEST_F(TestShaderCompileService, GroupRearmAfterTerminalSnapshotDeliversBothGenerations)
{
  store->Put(MakeKey("group-snapshot"), {1});
  auto group = service->RequestGroup(
      "fake", {Pass("group-snapshot", "group-snapshot")}, "preset", {});
  auto handle = group->GetHandles().front();
  ExpectTerminal(handle);

  CEvent snapshotTaken;
  CEvent releaseSnapshot{true};
  CShaderCompileServiceTestAccess::SetGroupBeforeCallbackMark(
      *service, group,
      [&snapshotTaken, &releaseSnapshot]
      {
        snapshotTaken.Set();
        releaseSnapshot.Wait();
      });
  std::atomic_uint completionCount{0};
  CEvent secondCompletion;
  auto addCallback = std::async(
      std::launch::async,
      [&]
      {
        group->AddCompletionCallback(
            [&]
            {
              if (++completionCount == 2)
                secondCompletion.Set();
            });
      });
  const bool snapshotWasTaken = snapshotTaken.Wait(5s);
  if (!snapshotWasTaken)
    releaseSnapshot.Set();
  ASSERT_TRUE(snapshotWasTaken);

  compiler->compileRelease.Reset();
  EXPECT_TRUE(service->RejectDiskArtifact(handle, handle->GetGeneration()));
  releaseSnapshot.Set();
  addCallback.get();
  EXPECT_EQ(1u, completionCount);
  compiler->compileRelease.Set();
  EXPECT_TRUE(secondCompletion.Wait(5s));
  EXPECT_EQ(2u, completionCount);
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
  const bool prepareEntered = localCompiler->prepareEntered.Wait(5s);
  if (!prepareEntered)
    localCompiler->prepareRelease.Set();
  ASSERT_TRUE(prepareEntered);
  localService.reset();
  localCompiler->prepareRelease.Set();
  SUCCEED();
}

TEST_F(TestShaderCompileService, RejectedJobSubmissionBecomesTerminalFailure)
{
  CServiceBroker::GetJobManager()->CancelJobs();
  auto handle = service->Request("fake", Pass("rejected", "n"), {});
  EXPECT_EQ(ShaderCompileState::FAILED, handle->GetState());
  EXPECT_TRUE(handle->GetTerminal());
  CServiceBroker::GetJobManager()->Restart();
  auto retry = service->Request("fake", Pass("rejected", "n"), {});
  ExpectTerminal(retry);
  EXPECT_EQ(ShaderCompileState::READY, retry->GetState());
}

TEST_F(TestShaderCompileService, AcceptedThenCanceledInitialJobBecomesTerminal)
{
  auto blockers = SaturateLowPriorityWorkers();
  auto handle = service->Request("fake", Pass("cancel-initial", "o"), {});
  auto cancel = std::async(std::launch::async,
                           [] { CServiceBroker::GetJobManager()->CancelJobs(); });
  ExpectTerminal(handle);
  EXPECT_EQ(ShaderCompileState::FAILED, handle->GetState());
  EXPECT_TRUE(handle->GetTerminal());
  blockers->release.Set();
  cancel.get();
  CServiceBroker::GetJobManager()->Restart();
  auto retry = service->Request("fake", Pass("cancel-initial", "o"), {});
  ExpectTerminal(retry);
  EXPECT_EQ(ShaderCompileState::READY, retry->GetState());
}

TEST_F(TestShaderCompileService, AcceptedThenCanceledDiskRetryBecomesTerminal)
{
  store->Put(MakeKey("p"), {1, 2, 3});
  auto handle = service->Request("fake", Pass("cancel-retry", "p"), {});
  ExpectTerminal(handle);
  const auto generation = handle->GetGeneration();

  auto blockers = SaturateLowPriorityWorkers();
  const bool retryQueued = service->RejectDiskArtifact(handle, generation);
  if (!retryQueued)
    blockers->release.Set();
  ASSERT_TRUE(retryQueued);
  auto cancel = std::async(std::launch::async,
                           [] { CServiceBroker::GetJobManager()->CancelJobs(); });
  ExpectTerminal(handle);
  EXPECT_EQ(ShaderCompileState::FAILED, handle->GetState());
  EXPECT_EQ(generation + 1, handle->GetTerminal()->generation);
  blockers->release.Set();
  cancel.get();
  CServiceBroker::GetJobManager()->Restart();
  auto fresh = service->Request("fake", Pass("cancel-retry", "p"), {});
  ExpectTerminal(fresh);
  EXPECT_EQ(ShaderCompileState::READY, fresh->GetState());
}
