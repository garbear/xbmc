/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderCompileService.h"

#include "jobs/JobQueue.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"

#include <map>
#include <mutex>
#include <string>
#include <utility>

namespace KODI::SHADER
{
namespace INTERNAL
{
struct RequestState;

struct CanonicalEntry
{
  mutable CCriticalSection mutex;
  ShaderCompileState state{ShaderCompileState::UNKNOWN};
  std::uint64_t generation{0};
  ShaderCompileKey key;
  std::shared_ptr<const ShaderCompiledArtifact> artifact;
  std::string error;
  std::shared_ptr<const IShaderPreparedUnit> prepared;
  std::shared_ptr<IShaderCompiler> compiler;
  std::shared_ptr<IShaderArtifactStore> store;
  bool diskRetrySpent{false};
  std::vector<std::weak_ptr<RequestState>> requests;
};

struct RequestState
{
  mutable CCriticalSection mutex;
  std::shared_ptr<CanonicalEntry> entry;
  ShaderRequestDisposition disposition{ShaderRequestDisposition::QUEUED};
  std::vector<std::function<void()>> callbacks;
};

struct CompilerRegistration
{
  std::shared_ptr<IShaderCompiler> compiler;
  std::shared_ptr<IShaderArtifactStore> store;
};

struct ServiceState
{
  mutable CCriticalSection mutex;
  bool stopping{false};
  std::map<std::string, std::weak_ptr<RequestState>, std::less<>> provisional;
  std::map<std::string, std::shared_ptr<CanonicalEntry>, std::less<>> canonical;
  std::map<std::string, std::shared_ptr<CanonicalEntry>, std::less<>> preparationFailures;
  std::map<std::string, CompilerRegistration, std::less<>> compilers;
  std::function<void()> beforeCanonicalAttach;
};

struct Dispatcher
{
  CCriticalSection mutex;
  CJobQueue* queue{nullptr};
  bool stopping{false};
};

struct GroupState
{
  mutable CCriticalSection mutex;
  std::vector<std::shared_ptr<CShaderCompileHandle>> handles;
  std::vector<std::function<void()>> callbacks;
  std::string lastCompletedGeneration;
};
} // namespace INTERNAL

namespace
{
using INTERNAL::CanonicalEntry;
using INTERNAL::Dispatcher;
using INTERNAL::RequestState;
using INTERNAL::ServiceState;

bool IsTerminal(ShaderCompileState state)
{
  return state == ShaderCompileState::READY || state == ShaderCompileState::FAILED;
}

std::shared_ptr<CanonicalEntry> GetEntry(const std::shared_ptr<RequestState>& request)
{
  std::unique_lock lock(request->mutex);
  return request->entry;
}

void AttachRequest(const std::shared_ptr<RequestState>& request,
                   const std::shared_ptr<CanonicalEntry>& entry,
                   ShaderRequestDisposition disposition)
{
  {
    std::unique_lock lock(request->mutex);
    request->entry = entry;
    request->disposition = disposition;
  }
  std::unique_lock lock(entry->mutex);
  entry->requests.emplace_back(request);
}

void NotifyRequest(const std::shared_ptr<RequestState>& request)
{
  std::vector<std::function<void()>> callbacks;
  {
    std::unique_lock lock(request->mutex);
    callbacks = request->callbacks;
  }
  for (auto& callback : callbacks)
    callback();
}

void NotifyEntry(const std::shared_ptr<CanonicalEntry>& entry)
{
  std::vector<std::shared_ptr<RequestState>> requests;
  {
    std::unique_lock lock(entry->mutex);
    for (auto it = entry->requests.begin(); it != entry->requests.end();)
    {
      if (auto request = it->lock())
      {
        requests.emplace_back(std::move(request));
        ++it;
      }
      else
        it = entry->requests.erase(it);
    }
  }
  for (const auto& request : requests)
    NotifyRequest(request);
}

template<typename F>
bool Submit(const std::shared_ptr<Dispatcher>& dispatcher, F&& function)
{
  std::unique_lock lock(dispatcher->mutex);
  if (dispatcher->stopping || dispatcher->queue == nullptr)
    return false;
  dispatcher->queue->Submit(std::forward<F>(function));
  return true;
}

void RemoveProvisional(const std::shared_ptr<ServiceState>& state,
                       const std::string& identity,
                       const std::shared_ptr<RequestState>& request)
{
  std::unique_lock lock(state->mutex);
  const auto it = state->provisional.find(identity);
  if (it != state->provisional.end() && it->second.lock() == request)
    state->provisional.erase(it);
}

void SetTerminal(const std::shared_ptr<CanonicalEntry>& entry,
                 ShaderCompileState state,
                 std::shared_ptr<const ShaderCompiledArtifact> artifact,
                 std::string error)
{
  {
    std::unique_lock lock(entry->mutex);
    entry->state = state;
    entry->artifact = std::move(artifact);
    entry->error = std::move(error);
  }
  NotifyEntry(entry);
}

void CompileEntry(const std::shared_ptr<CanonicalEntry>& entry)
{
  std::shared_ptr<IShaderCompiler> compiler;
  std::shared_ptr<const IShaderPreparedUnit> prepared;
  std::shared_ptr<IShaderArtifactStore> store;
  ShaderCompileKey key;
  {
    std::unique_lock lock(entry->mutex);
    entry->state = ShaderCompileState::COMPILING;
    compiler = entry->compiler;
    prepared = entry->prepared;
    store = entry->store;
    key = entry->key;
  }

  const ShaderCompileResult result = compiler->Compile(*prepared);
  if (!result.error.empty() || result.bytecode.empty())
  {
    SetTerminal(entry, ShaderCompileState::FAILED, {},
                result.error.empty() ? "Shader compiler returned no bytecode" : result.error);
    return;
  }

  auto bytes = std::make_shared<const std::vector<std::uint8_t>>(result.bytecode);
  auto artifact = std::make_shared<const ShaderCompiledArtifact>(
      ShaderCompiledArtifact{key, bytes, ShaderArtifactOrigin::COMPILED});
  if (store)
    store->Store(key, *bytes);
  SetTerminal(entry, ShaderCompileState::READY, std::move(artifact), {});
}

void PrepareRequest(const std::shared_ptr<ServiceState>& state,
                    const std::shared_ptr<IShaderCompiler>& compiler,
                    const std::shared_ptr<IShaderArtifactStore>& store,
                    const std::shared_ptr<const IShaderCompileInput>& input,
                    const std::shared_ptr<RequestState>& request,
                    const std::string& provisionalIdentity,
                    const std::string& backendId)
{
  auto provisionalEntry = GetEntry(request);
  {
    std::unique_lock lock(provisionalEntry->mutex);
    provisionalEntry->state = ShaderCompileState::COMPILING;
  }

  const ShaderPrepareResult prepared = compiler->Prepare(*input);
  if (!prepared.canonicalKey || !prepared.prepared)
  {
    const std::string fingerprint = backendId + "\n" + prepared.failureFingerprint;
    std::shared_ptr<CanonicalEntry> failureEntry;
    bool firstFailure{false};
    {
      std::unique_lock lock(state->mutex);
      const auto it = state->preparationFailures.find(fingerprint);
      if (it != state->preparationFailures.end())
        failureEntry = it->second;
      else
      {
        failureEntry = provisionalEntry;
        state->preparationFailures.emplace(fingerprint, failureEntry);
        firstFailure = true;
      }
    }

    if (failureEntry != provisionalEntry)
      AttachRequest(request, failureEntry, ShaderRequestDisposition::MEMORY_HIT);
    else
      SetTerminal(failureEntry, ShaderCompileState::FAILED, {},
                  prepared.error.empty() ? "Shader preparation failed" : prepared.error);

    RemoveProvisional(state, provisionalIdentity, request);
    if (failureEntry != provisionalEntry)
      NotifyRequest(request);
    if (firstFailure)
      CLog::Log(LOGERROR, "Shader preparation failed: {}", prepared.error);
    return;
  }

  const std::string canonicalIdentity = backendId + "\n" + prepared.canonicalKey->hex;
  std::shared_ptr<CanonicalEntry> entry;
  bool ownsCanonical{false};
  ShaderCompileState existingState{ShaderCompileState::UNKNOWN};
  {
    std::unique_lock lock(state->mutex);
    const auto it = state->canonical.find(canonicalIdentity);
    if (it != state->canonical.end())
      entry = it->second;
    else
    {
      entry = provisionalEntry;
      state->canonical.emplace(canonicalIdentity, entry);
      ownsCanonical = true;
    }
  }

  if (!ownsCanonical)
  {
    bool terminal{false};
    {
      std::unique_lock requestLock(request->mutex);
      std::unique_lock entryLock(entry->mutex);
      existingState = entry->state;
      if (state->beforeCanonicalAttach)
        state->beforeCanonicalAttach();
      terminal = IsTerminal(existingState);
      request->entry = entry;
      request->disposition =
          terminal ? ShaderRequestDisposition::MEMORY_HIT : ShaderRequestDisposition::QUEUED;
      entry->requests.emplace_back(request);
    }
    RemoveProvisional(state, provisionalIdentity, request);
    if (terminal)
      NotifyRequest(request);
    return;
  }

  {
    std::unique_lock lock(entry->mutex);
    entry->key = *prepared.canonicalKey;
    entry->prepared = prepared.prepared;
    entry->compiler = compiler;
    entry->store = store;
  }

  if (store)
  {
    const ShaderCacheLoadResult cached = store->Load(*prepared.canonicalKey);
    if (cached.state == ShaderCacheLoadState::HIT && cached.bytecode)
    {
      {
        std::unique_lock lock(request->mutex);
        request->disposition = ShaderRequestDisposition::DISK_HIT;
      }
      auto artifact = std::make_shared<const ShaderCompiledArtifact>(ShaderCompiledArtifact{
          *prepared.canonicalKey, cached.bytecode, ShaderArtifactOrigin::DISK});
      SetTerminal(entry, ShaderCompileState::READY, std::move(artifact), {});
      RemoveProvisional(state, provisionalIdentity, request);
      return;
    }
  }

  CompileEntry(entry);
  RemoveProvisional(state, provisionalIdentity, request);
}

ShaderPresetState GetGroupState(const std::vector<std::shared_ptr<CShaderCompileHandle>>& handles)
{
  bool failed{false};
  for (const auto& handle : handles)
  {
    const ShaderCompileState state = handle->GetState();
    if (!IsTerminal(state))
      return ShaderPresetState::PENDING;
    if (state == ShaderCompileState::FAILED || !handle->GetArtifact())
      failed = true;
  }
  return failed ? ShaderPresetState::FAILED : ShaderPresetState::READY;
}

void EvaluateGroup(const std::shared_ptr<INTERNAL::GroupState>& group)
{
  std::vector<std::shared_ptr<CShaderCompileHandle>> handles;
  {
    std::unique_lock lock(group->mutex);
    handles = group->handles;
  }
  if (GetGroupState(handles) == ShaderPresetState::PENDING)
    return;

  std::string generation;
  for (const auto& handle : handles)
    generation += std::to_string(handle->GetGeneration()) + ":";

  std::vector<std::function<void()>> callbacks;
  {
    std::unique_lock lock(group->mutex);
    if (group->lastCompletedGeneration == generation)
      return;
    group->lastCompletedGeneration = std::move(generation);
    callbacks = group->callbacks;
  }
  for (auto& callback : callbacks)
    callback();
}
} // namespace

CShaderCompileHandle::CShaderCompileHandle(std::shared_ptr<INTERNAL::RequestState> state)
  : m_state(std::move(state))
{
}

ShaderCompileState CShaderCompileHandle::GetState() const
{
  const auto entry = GetEntry(m_state);
  std::unique_lock lock(entry->mutex);
  return entry->state;
}

std::uint64_t CShaderCompileHandle::GetGeneration() const
{
  const auto entry = GetEntry(m_state);
  std::unique_lock lock(entry->mutex);
  return entry->generation;
}

ShaderRequestDisposition CShaderCompileHandle::GetDisposition() const
{
  std::unique_lock lock(m_state->mutex);
  return m_state->disposition;
}

std::shared_ptr<const ShaderCompiledArtifact> CShaderCompileHandle::GetArtifact() const
{
  const auto entry = GetEntry(m_state);
  std::unique_lock lock(entry->mutex);
  return entry->artifact;
}

std::string CShaderCompileHandle::GetError() const
{
  const auto entry = GetEntry(m_state);
  std::unique_lock lock(entry->mutex);
  return entry->error;
}

void CShaderCompileHandle::AddCompletionCallback(std::function<void()> callback) const
{
  bool terminal{false};
  {
    std::unique_lock lock(m_state->mutex);
    m_state->callbacks.emplace_back(callback);
    std::unique_lock entryLock(m_state->entry->mutex);
    terminal = IsTerminal(m_state->entry->state);
  }
  if (terminal)
    callback();
}

CShaderCompileService::CShaderCompileService() : CShaderCompileService(std::function<void()>{})
{
}

CShaderCompileService::CShaderCompileService(std::function<void()> beforeCanonicalAttach)
  : m_state(std::make_shared<INTERNAL::ServiceState>()),
    m_dispatcher(std::make_shared<INTERNAL::Dispatcher>()),
    m_queue(std::make_unique<CJobQueue>(false, 2, CJob::PRIORITY_LOW))
{
  m_state->beforeCanonicalAttach = std::move(beforeCanonicalAttach);
  m_dispatcher->queue = m_queue.get();
}

CShaderCompileService::~CShaderCompileService()
{
  {
    std::unique_lock lock(m_state->mutex);
    m_state->stopping = true;
  }
  {
    std::unique_lock lock(m_dispatcher->mutex);
    m_dispatcher->stopping = true;
    m_dispatcher->queue = nullptr;
  }
  m_queue->CancelJobs();
}

void CShaderCompileService::RegisterCompiler(std::shared_ptr<IShaderCompiler> compiler,
                                             std::shared_ptr<IShaderArtifactStore> store)
{
  if (!compiler)
    return;
  const std::string backendId{compiler->GetBackendId()};
  std::unique_lock lock(m_state->mutex);
  m_state->compilers[backendId] = {std::move(compiler), std::move(store)};
}

bool CShaderCompileService::SupportsAsyncCompilation(std::string_view backendId) const
{
  std::unique_lock lock(m_state->mutex);
  return m_state->compilers.contains(backendId);
}

std::shared_ptr<CShaderCompileHandle> CShaderCompileService::Request(std::string_view backendId,
                                                                     const ShaderPass& pass,
                                                                     ShaderCompileContext context)
{
  INTERNAL::CompilerRegistration registration;
  {
    std::unique_lock lock(m_state->mutex);
    const auto it = m_state->compilers.find(backendId);
    if (it != m_state->compilers.end())
      registration = it->second;
  }

  if (!registration.compiler)
  {
    auto entry = std::make_shared<CanonicalEntry>();
    entry->state = ShaderCompileState::FAILED;
    entry->error = "Unsupported shader compiler backend";
    auto request = std::make_shared<RequestState>();
    request->entry = entry;
    entry->requests.emplace_back(request);
    return std::shared_ptr<CShaderCompileHandle>(new CShaderCompileHandle(std::move(request)));
  }

  ShaderCompileRequest compileRequest =
      registration.compiler->CreateRequest(pass, std::move(context));
  const std::string provisionalIdentity =
      std::string{backendId} + "\n" + compileRequest.provisionalKey;

  std::shared_ptr<RequestState> request;
  std::shared_ptr<RequestState> provisionalOwner;
  bool created{false};
  {
    std::unique_lock lock(m_state->mutex);
    if (const auto it = m_state->provisional.find(provisionalIdentity);
        it != m_state->provisional.end())
      provisionalOwner = it->second.lock();

    if (!provisionalOwner)
    {
      auto entry = std::make_shared<CanonicalEntry>();
      entry->state = ShaderCompileState::QUEUED;
      request = std::make_shared<RequestState>();
      request->entry = entry;
      entry->requests.emplace_back(request);
      m_state->provisional[provisionalIdentity] = request;
      created = true;
    }
  }

  if (provisionalOwner)
  {
    request = std::make_shared<RequestState>();
    AttachRequest(request, GetEntry(provisionalOwner), ShaderRequestDisposition::MEMORY_HIT);
  }

  auto handle = std::shared_ptr<CShaderCompileHandle>(new CShaderCompileHandle(request));
  if (created)
  {
    const auto state = m_state;
    const auto compiler = registration.compiler;
    const auto store = registration.store;
    const auto input = compileRequest.input;
    const std::string backend{backendId};
    if (!Submit(m_dispatcher,
                [state, compiler, store, input, request, provisionalIdentity, backend]
                {
                  PrepareRequest(state, compiler, store, input, request, provisionalIdentity,
                                 backend);
                }))
      SetTerminal(GetEntry(request), ShaderCompileState::FAILED, {}, "Shader service is stopping");
  }
  return handle;
}

std::shared_ptr<CShaderCompileGroup> CShaderCompileService::RequestGroup(
    std::string_view backendId,
    const std::vector<ShaderPass>& passes,
    std::string presetPath,
    std::function<void()> completion)
{
  auto state = std::make_shared<INTERNAL::GroupState>();
  for (unsigned int index = 0; index < passes.size(); ++index)
  {
    const ShaderPass& pass = passes[index];
    state->handles.emplace_back(
        Request(backendId, pass, {presetPath, index, pass.alias, pass.sourcePath}));
  }
  if (completion)
    state->callbacks.emplace_back(std::move(completion));

  std::weak_ptr<INTERNAL::GroupState> weakState = state;
  for (const auto& handle : state->handles)
    handle->AddCompletionCallback(
        [weakState]
        {
          if (const auto locked = weakState.lock())
            EvaluateGroup(locked);
        });
  EvaluateGroup(state);
  return std::shared_ptr<CShaderCompileGroup>(new CShaderCompileGroup(std::move(state)));
}

bool CShaderCompileService::RejectDiskArtifact(const std::shared_ptr<CShaderCompileHandle>& handle,
                                               std::uint64_t generation)
{
  if (!handle)
    return false;
  const auto entry = GetEntry(handle->m_state);
  std::shared_ptr<IShaderArtifactStore> store;
  ShaderCompileKey key;
  {
    std::unique_lock lock(entry->mutex);
    if (entry->generation != generation || entry->diskRetrySpent || !entry->artifact ||
        entry->artifact->origin != ShaderArtifactOrigin::DISK || !entry->prepared ||
        !entry->compiler)
      return false;
    entry->diskRetrySpent = true;
    ++entry->generation;
    entry->artifact.reset();
    entry->error.clear();
    entry->state = ShaderCompileState::QUEUED;
    store = entry->store;
    key = entry->key;
  }

  if (store)
    store->Remove(key);
  return Submit(m_dispatcher, [entry] { CompileEntry(entry); });
}

CShaderCompileGroup::CShaderCompileGroup(std::shared_ptr<INTERNAL::GroupState> state)
  : m_state(std::move(state))
{
}

ShaderPresetState CShaderCompileGroup::GetState() const
{
  return GetGroupState(GetHandles());
}

std::vector<std::shared_ptr<CShaderCompileHandle>> CShaderCompileGroup::GetHandles() const
{
  std::unique_lock lock(m_state->mutex);
  return m_state->handles;
}

void CShaderCompileGroup::AddCompletionCallback(std::function<void()> callback) const
{
  {
    std::unique_lock lock(m_state->mutex);
    m_state->callbacks.emplace_back(callback);
  }
  EvaluateGroup(m_state);
}
} // namespace KODI::SHADER
