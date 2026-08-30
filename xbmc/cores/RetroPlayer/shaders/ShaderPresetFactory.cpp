/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetFactory.h"

#include "IShaderPresetLoader.h"
#include "ShaderArtifactStore.h"
#include "ShaderCompileService.h"
#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/ShaderPreset.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/binary-addons/BinaryAddonManager.h"
#include "jobs/JobQueue.h"
#include "jobs/LambdaJob.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>

#if defined(HAS_DX)
#include "cores/RetroPlayer/shaders/windows/ShaderCompilerDX.h"
#endif

using namespace KODI::SHADER;

namespace KODI::SHADER::INTERNAL
{
class CShaderPresetLoaderSlot
{
public:
  explicit CShaderPresetLoaderSlot(std::shared_ptr<IShaderPresetLoader> loader)
    : m_loader(std::move(loader))
  {
  }

  std::uint64_t Generation() const
  {
    std::unique_lock lock(m_mutex);
    return m_generation;
  }

  bool Load(std::uint64_t expectedGeneration,
            std::string_view presetPath,
            ShaderPresetDefinition& definition)
  {
    std::shared_ptr<IShaderPresetLoader> loader;
    {
      std::unique_lock lock(m_mutex);
      if (!m_accepting || expectedGeneration != m_generation || !m_loader)
        return false;
      ++m_inFlight;
      loader = m_loader;
    }

    ShaderPresetDefinition localDefinition;
    bool result{false};
    try
    {
      result = loader->LoadPreset(presetPath, localDefinition);
    }
    catch (...)
    {
      loader.reset();
      FinishLoad(expectedGeneration);
      throw;
    }
    loader.reset();
    const bool generationStillActive = FinishLoad(expectedGeneration);
    if (result && generationStillActive)
      definition = std::move(localDefinition);
    return result && generationStillActive;
  }

  void BeginDeactivate()
  {
    std::unique_lock lock(m_mutex);
    m_accepting = false;
    ++m_generation;
  }

  void Drain()
  {
    std::unique_lock lock(m_mutex);
    m_drained.wait(lock, [this] { return m_inFlight == 0; });
    m_loader.reset();
  }

  bool Owns(const std::shared_ptr<IShaderPresetLoader>& loader) const
  {
    std::unique_lock lock(m_mutex);
    return m_loader == loader;
  }

private:
  bool FinishLoad(std::uint64_t expectedGeneration)
  {
    std::unique_lock lock(m_mutex);
    const bool active = m_accepting && expectedGeneration == m_generation;
    --m_inFlight;
    if (m_inFlight == 0)
      m_drained.notify_all();
    return active;
  }

  mutable std::mutex m_mutex;
  std::condition_variable m_drained;
  std::shared_ptr<IShaderPresetLoader> m_loader;
  std::uint64_t m_generation{1};
  std::size_t m_inFlight{0};
  bool m_accepting{true};
};

struct LoaderRef
{
  std::shared_ptr<CShaderPresetLoaderSlot> slot;
  std::uint64_t generation{0};
};

using LoaderMap = std::map<std::string, LoaderRef, std::less<>>;

struct ShaderPresetLoaderRegistry
{
  std::mutex updateMutex;
  mutable std::shared_mutex mutex;
  std::shared_ptr<const LoaderMap> snapshot{std::make_shared<const LoaderMap>()};
};

struct ShaderWarmupRun;

struct ShaderWarmupState
{
  ShaderWarmupState(std::shared_ptr<ShaderPresetLoaderRegistry> registry_,
                    std::shared_ptr<CShaderCompileService> compileService_)
    : registry(std::move(registry_)),
      compileService(std::move(compileService_)),
      queue(false, 1, CJob::PRIORITY_LOW)
  {
  }

  std::shared_ptr<ShaderPresetLoaderRegistry> registry;
  std::shared_ptr<CShaderCompileService> compileService;
  CJobQueue queue;
  std::mutex mutex;
  std::map<std::string, std::shared_ptr<ShaderWarmupRun>, std::less<>> active;
  std::set<std::shared_ptr<ShaderWarmupRun>, std::owner_less<std::shared_ptr<ShaderWarmupRun>>>
      runs;
  std::function<void(const ShaderWarmupSummary&)> summaryCallback;
  std::function<void(const std::shared_ptr<CShaderCompileHandle>&)> requestCallback;
  bool stopping{false};
};

struct ShaderWarmupSlot
{
  bool consumed{false};
};

struct ShaderWarmupRun
{
  std::shared_ptr<ShaderWarmupState> state;
  std::string signature;
  std::string backendId;
  std::vector<std::string> presetPaths;
  std::mutex mutex;
  ShaderWarmupSummary summary;
  std::size_t pending{0};
  bool enumerationFinished{false};
  bool finalized{false};
  std::vector<ShaderCompileTerminal> terminals;
  std::vector<std::shared_ptr<CShaderCompileHandle>> handles;
};
} // namespace KODI::SHADER::INTERNAL

namespace
{
std::string CanonicalExtension(std::string extension)
{
  if (!extension.empty() && extension.front() != '.')
    extension.insert(extension.begin(), '.');
  return extension;
}

bool LoadFromRegistry(
    const std::shared_ptr<KODI::SHADER::INTERNAL::ShaderPresetLoaderRegistry>& registry,
    std::string_view presetPath,
    ShaderPresetDefinition& definition)
{
  KODI::SHADER::INTERNAL::LoaderRef loader;
  {
    const std::string extension = URIUtils::GetExtension(std::string{presetPath});
    std::shared_lock lock(registry->mutex);
    const auto it = registry->snapshot->find(extension);
    if (it == registry->snapshot->end())
      return false;
    loader = it->second;
  }
  return loader.slot->Load(loader.generation, presetPath, definition);
}

std::string WarmupSignature(std::string_view backendId,
                            const std::vector<std::string>& presetPaths)
{
  std::string signature = std::to_string(backendId.size()) + ":" + std::string{backendId};
  for (const auto& path : presetPaths)
    signature += std::to_string(path.size()) + ":" + path;
  return signature;
}

int ClassificationPriority(const ShaderCompileTerminal& terminal)
{
  if (terminal.state == ShaderCompileState::FAILED)
    return 4;
  if (terminal.disposition == ShaderRequestDisposition::DISK_HIT)
    return 3;
  if (terminal.disposition == ShaderRequestDisposition::COALESCED &&
      terminal.artifactOrigin == ShaderArtifactOrigin::DISK)
    return 3;
  if (terminal.disposition == ShaderRequestDisposition::QUEUED ||
      terminal.disposition == ShaderRequestDisposition::COALESCED)
    return 2;
  return 1;
}

void FinalizeWarmup(const std::shared_ptr<KODI::SHADER::INTERNAL::ShaderWarmupRun>& run)
{
  ShaderWarmupSummary summary;
  {
    std::unique_lock lock(run->mutex);
    if (run->finalized || !run->enumerationFinished || run->pending != 0)
      return;
    run->finalized = true;
    summary = run->summary;
    std::map<ShaderCompileIdentity, ShaderCompileTerminal> unique;
    for (const auto& terminal : run->terminals)
    {
      const auto [it, inserted] = unique.try_emplace(terminal.identity, terminal);
      if (!inserted && ClassificationPriority(terminal) > ClassificationPriority(it->second))
        it->second = terminal;
    }
    summary.unique = unique.size();
    for (const auto& [identity, terminal] : unique)
    {
      switch (ClassificationPriority(terminal))
      {
        case 4:
          ++summary.failed;
          break;
        case 3:
          ++summary.diskHits;
          break;
        case 2:
          ++summary.queued;
          break;
        default:
          ++summary.memoryHits;
          break;
      }
    }
    run->handles.clear();
  }

  std::function<void(const ShaderWarmupSummary&)> callback;
  {
    std::unique_lock lock(run->state->mutex);
    if (run->state->stopping)
      return;
    const auto it = run->state->active.find(run->signature);
    if (it != run->state->active.end() && it->second == run)
      run->state->active.erase(it);
    run->state->runs.erase(run);
    callback = run->state->summaryCallback;
  }

  CLog::Log(LOGDEBUG,
            "Video shader warmup: {} presets, {} passes, {} unique, {} memory, {} disk, {} "
            "queued, {} failed",
            summary.presets, summary.passes, summary.unique, summary.memoryHits,
            summary.diskHits, summary.queued, summary.failed);
  if (callback)
    callback(summary);
}

void ConsumeWarmupHandle(
    const std::weak_ptr<KODI::SHADER::INTERNAL::ShaderWarmupRun>& weakRun,
    const std::shared_ptr<KODI::SHADER::INTERNAL::ShaderWarmupSlot>& slot,
    const std::weak_ptr<CShaderCompileHandle>& weakHandle)
{
  const auto run = weakRun.lock();
  const auto handle = weakHandle.lock();
  if (!run || !handle)
    return;
  const auto terminal = handle->GetTerminal();
  if (!terminal)
    return;
  {
    std::unique_lock lock(run->mutex);
    if (slot->consumed)
      return;
    slot->consumed = true;
    run->terminals.emplace_back(*terminal);
    --run->pending;
  }
  FinalizeWarmup(run);
}

void EnumerateWarmup(const std::shared_ptr<KODI::SHADER::INTERNAL::ShaderWarmupRun>& run)
{
  for (const auto& presetPath : run->presetPaths)
  {
    {
      std::unique_lock lock(run->state->mutex);
      if (run->state->stopping)
        return;
    }
    ShaderPresetDefinition definition;
    bool loaded{false};
    try
    {
      loaded = LoadFromRegistry(run->state->registry, presetPath, definition);
    }
    catch (const std::exception& exception)
    {
      CLog::Log(LOGERROR, "Video shader warmup: exception parsing preset '{}': {}", presetPath,
                exception.what());
      continue;
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Video shader warmup: unknown exception parsing preset '{}'",
                presetPath);
      continue;
    }
    if (!loaded)
    {
      CLog::Log(LOGERROR, "Video shader warmup: failed to parse preset '{}'", presetPath);
      continue;
    }
    {
      std::unique_lock lock(run->mutex);
      run->summary.passes += definition.passes.size();
    }
    for (unsigned int index = 0; index < definition.passes.size(); ++index)
    {
      const ShaderPass& pass = definition.passes[index];
      auto slot = std::make_shared<KODI::SHADER::INTERNAL::ShaderWarmupSlot>();
      {
        std::unique_lock lock(run->mutex);
        ++run->pending;
      }
      auto handle = run->state->compileService->Request(
          run->backendId, pass, {presetPath, index, pass.alias, pass.sourcePath});
      {
        std::unique_lock lock(run->mutex);
        run->handles.emplace_back(handle);
      }
      std::weak_ptr<KODI::SHADER::INTERNAL::ShaderWarmupRun> weakRun = run;
      std::weak_ptr<CShaderCompileHandle> weakHandle = handle;
      handle->AddCompletionCallback(
          [weakRun, slot, weakHandle] { ConsumeWarmupHandle(weakRun, slot, weakHandle); });
      std::function<void(const std::shared_ptr<CShaderCompileHandle>&)> requestCallback;
      {
        std::unique_lock lock(run->state->mutex);
        requestCallback = run->state->requestCallback;
      }
      if (requestCallback)
        requestCallback(handle);
    }
  }
  {
    std::unique_lock lock(run->mutex);
    run->enumerationFinished = true;
  }
  {
    std::unique_lock lock(run->state->mutex);
    const auto it = run->state->active.find(run->signature);
    if (it != run->state->active.end() && it->second == run)
      run->state->active.erase(it);
  }
  FinalizeWarmup(run);
}

void DiscardWarmup(const std::shared_ptr<KODI::SHADER::INTERNAL::ShaderWarmupRun>& run)
{
  std::unique_lock lock(run->state->mutex);
  const auto it = run->state->active.find(run->signature);
  if (it != run->state->active.end() && it->second == run)
    run->state->active.erase(it);
  run->state->runs.erase(run);
}

class CDiscardAwareWarmupJob final : public CJob
{
public:
  explicit CDiscardAwareWarmupJob(
      std::shared_ptr<KODI::SHADER::INTERNAL::ShaderWarmupRun> run)
    : m_run(std::move(run))
  {
  }

  ~CDiscardAwareWarmupJob() override
  {
    if (!m_handled)
      DiscardWarmup(m_run);
  }

  bool DoWork() override
  {
    try
    {
      EnumerateWarmup(m_run);
      m_handled = true;
    }
    catch (...)
    {
      DiscardWarmup(m_run);
      m_handled = true;
      throw;
    }
    return true;
  }

private:
  std::shared_ptr<KODI::SHADER::INTERNAL::ShaderWarmupRun> m_run;
  bool m_handled{false};
};
} // namespace

CShaderPresetFactory::CShaderPresetFactory(ADDON::CAddonMgr& addons)
  : m_addons(&addons),
    m_compileService(std::make_shared<CShaderCompileService>()),
    m_registry(std::make_shared<INTERNAL::ShaderPresetLoaderRegistry>()),
    m_warmup(std::make_shared<INTERNAL::ShaderWarmupState>(m_registry, m_compileService))
{
#if defined(HAS_DX)
  m_compileService->RegisterCompiler(
      std::make_shared<CShaderCompilerDX>(),
      std::make_shared<CShaderArtifactStore>("special://temp/retroplayer/shaders/dx11/v1/", ".fxc",
                                             1, 64ULL * 1024ULL * 1024ULL));
#endif
  UpdateAddons();

  m_addons->Events().Subscribe(this,
                              [this](const ADDON::AddonEvent& event)
                              {
                                if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
                                    typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
                                    typeid(event) == typeid(ADDON::AddonEvents::UnInstalled) ||
                                    typeid(event) == typeid(ADDON::AddonEvents::ReInstalled))
                                {
                                  UpdateAddons(typeid(event) == typeid(ADDON::AddonEvents::ReInstalled)
                                                   ? event.addonId
                                                   : std::string_view{});
                                }
                              });
}

CShaderPresetFactory::CShaderPresetFactory()
  : m_compileService(std::make_shared<CShaderCompileService>()),
    m_registry(std::make_shared<INTERNAL::ShaderPresetLoaderRegistry>()),
    m_warmup(std::make_shared<INTERNAL::ShaderWarmupState>(m_registry, m_compileService))
{
}

CShaderCompileService& CShaderPresetFactory::CompileService()
{
  return *m_compileService;
}

CShaderPresetFactory::~CShaderPresetFactory()
{
  if (m_addons)
    m_addons->Events().Unsubscribe(this);

  {
    std::unique_lock lock(m_warmup->mutex);
    m_warmup->stopping = true;
    m_warmup->summaryCallback = {};
    m_warmup->requestCallback = {};
    m_warmup->active.clear();
    m_warmup->runs.clear();
  }
  m_warmup->queue.CancelJobs();

  std::vector<std::shared_ptr<INTERNAL::CShaderPresetLoaderSlot>> slots;
  {
    std::unique_lock lock(m_registry->mutex);
    for (const auto& [extension, ref] : *m_registry->snapshot)
      if (std::ranges::find(slots, ref.slot) == slots.end())
        slots.emplace_back(ref.slot);
    m_registry->snapshot = std::make_shared<const INTERNAL::LoaderMap>();
  }
  for (const auto& slot : slots)
    slot->BeginDeactivate();
  for (const auto& slot : slots)
    slot->Drain();
}

void CShaderPresetFactory::SetWarmupSummaryCallback(
    std::function<void(const ShaderWarmupSummary&)> callback)
{
  std::unique_lock lock(m_warmup->mutex);
  m_warmup->summaryCallback = std::move(callback);
}

void CShaderPresetFactory::SetWarmupRequestCallback(
    std::function<void(const std::shared_ptr<CShaderCompileHandle>&)> callback)
{
  std::unique_lock lock(m_warmup->mutex);
  m_warmup->requestCallback = std::move(callback);
}

void CShaderPresetFactory::BlockReinstall(std::string addonId)
{
  m_blockedReinstallIds.emplace(std::move(addonId));
}

void CShaderPresetFactory::ClearReinstallBlock(std::string_view addonId)
{
  const auto it = m_blockedReinstallIds.find(addonId);
  if (it != m_blockedReinstallIds.end())
    m_blockedReinstallIds.erase(it);
}

bool CShaderPresetFactory::IsReinstallBlocked(std::string_view addonId) const
{
  return m_blockedReinstallIds.contains(addonId);
}

bool CShaderPresetFactory::ShouldSkipBlockedReinstall(std::string_view addonId,
                                                       bool oldGenerationActive)
{
  if (!IsReinstallBlocked(addonId))
    return false;
  if (oldGenerationActive)
    return true;
  ClearReinstallBlock(addonId);
  return false;
}

void CShaderPresetFactory::WarmupPresets(std::string backendId,
                                         std::vector<std::string> presetPaths)
{
  if (!m_compileService->SupportsAsyncCompilation(backendId))
    return;

  std::ranges::sort(presetPaths);
  presetPaths.erase(std::ranges::unique(presetPaths).begin(), presetPaths.end());
  const std::string signature = WarmupSignature(backendId, presetPaths);
  auto run = std::make_shared<INTERNAL::ShaderWarmupRun>();
  run->state = m_warmup;
  run->signature = signature;
  run->backendId = std::move(backendId);
  run->presetPaths = std::move(presetPaths);
  run->summary.presets = run->presetPaths.size();
  {
    std::unique_lock lock(m_warmup->mutex);
    if (m_warmup->stopping)
      return;
    if (m_warmup->active.contains(signature))
      return;
    m_warmup->active.emplace(signature, run);
    m_warmup->runs.emplace(run);
  }

  if (!m_warmup->queue.AddJob(new CDiscardAwareWarmupJob(run)))
  {
    std::unique_lock lock(m_warmup->mutex);
    const auto it = m_warmup->active.find(signature);
    if (it != m_warmup->active.end() && it->second == run)
      m_warmup->active.erase(it);
    m_warmup->runs.erase(run);
  }
}

void CShaderPresetFactory::PublishLoader(const std::shared_ptr<IShaderPresetLoader>& loader,
                                         const std::vector<std::string>& extensions)
{
  if (!loader)
    return;
  std::unique_lock updateLock(m_registry->updateMutex);
  auto slot = std::make_shared<INTERNAL::CShaderPresetLoaderSlot>(loader);
  std::unique_lock lock(m_registry->mutex);
  auto next = std::make_shared<INTERNAL::LoaderMap>(*m_registry->snapshot);
  for (const auto& extension : extensions)
  {
    const std::string canonical = CanonicalExtension(extension);
    if (!canonical.empty())
      next->try_emplace(canonical, INTERNAL::LoaderRef{slot, slot->Generation()});
  }
  m_registry->snapshot = std::move(next);
}

void CShaderPresetFactory::RemoveLoader(const std::shared_ptr<IShaderPresetLoader>& loader)
{
  std::unique_lock updateLock(m_registry->updateMutex);
  std::vector<std::shared_ptr<INTERNAL::CShaderPresetLoaderSlot>> slots;
  {
    std::shared_lock lock(m_registry->mutex);
    for (const auto& [extension, ref] : *m_registry->snapshot)
      if (ref.slot->Owns(loader) && std::ranges::find(slots, ref.slot) == slots.end())
        slots.emplace_back(ref.slot);
  }
  for (const auto& slot : slots)
    slot->BeginDeactivate();
  {
    std::unique_lock lock(m_registry->mutex);
    auto next = std::make_shared<INTERNAL::LoaderMap>(*m_registry->snapshot);
    std::erase_if(*next, [&slots](const auto& item)
                  { return std::ranges::find(slots, item.second.slot) != slots.end(); });
    m_registry->snapshot = std::move(next);
  }
  for (const auto& slot : slots)
    slot->Drain();
}

void CShaderPresetFactory::ReplaceLoader(
    const std::shared_ptr<IShaderPresetLoader>& oldLoader,
    const std::shared_ptr<IShaderPresetLoader>& newLoader,
    const std::vector<std::string>& extensions)
{
  RemoveLoader(oldLoader);
  PublishLoader(newLoader, extensions);
}

bool CShaderPresetFactory::HasAddons() const
{
  std::unique_lock lock(m_addonMutex);
  return !m_shaderAddons.empty() || !m_failedAddons.empty();
}

bool CShaderPresetFactory::LoadPreset(std::string_view presetPath,
                                      ShaderPresetDefinition& definition) const
{
  INTERNAL::LoaderRef loader;
  {
    const std::string extension = URIUtils::GetExtension(std::string{presetPath});
    std::shared_lock lock(m_registry->mutex);
    const auto it = m_registry->snapshot->find(extension);
    if (it == m_registry->snapshot->end())
      return false;
    loader = it->second;
  }
  return loader.slot->Load(loader.generation, presetPath, definition);
}

bool CShaderPresetFactory::CanLoadPreset(const std::string& presetPath) const
{
  const std::string extension = URIUtils::GetExtension(presetPath);
  std::shared_lock lock(m_registry->mutex);
  return !extension.empty() && m_registry->snapshot->contains(extension);
}

void CShaderPresetFactory::UpdateAddons(std::string_view reinstallId)
{
  using namespace ADDON;
  std::unique_lock reconcileLock(m_reconcileMutex);

  std::vector<AddonInfoPtr> addonInfo;
  m_addons->GetAddonInfos(addonInfo, true, AddonType::SHADERDLL);

  if (!reinstallId.empty())
  {
    std::shared_ptr<CShaderPresetAddon> old;
    std::shared_ptr<CShaderPresetAddon> failed;
    {
      std::unique_lock lock(m_addonMutex);
      if (const auto it = m_shaderAddons.find(reinstallId); it != m_shaderAddons.end())
      {
        old = std::move(it->second);
        m_shaderAddons.erase(it);
      }
      if (const auto it = m_failedAddons.find(reinstallId); it != m_failedAddons.end())
      {
        failed = std::move(it->second);
        m_failedAddons.erase(it);
      }
    }
    if (old)
      RemoveLoader(old);
    old.reset();
    failed.reset();
    if (CServiceBroker::GetBinaryAddonManager().GetRunningAddonBase(std::string{reinstallId}))
    {
      BlockReinstall(std::string{reinstallId});
      CLog::Log(LOGERROR,
                "Cannot reload shader preset add-on '{}': old binary generation is active",
                reinstallId);
      m_events.Publish(ShaderPresetLoadersChanged{});
      return;
    }
    ClearReinstallBlock(reinstallId);
  }

  // Look for removed/disabled add-ons
  std::vector<std::shared_ptr<CShaderPresetAddon>> removed;
  {
    std::unique_lock lock(m_addonMutex);
    for (auto it = m_shaderAddons.begin(); it != m_shaderAddons.end();)
    {
      const bool disabled =
          std::ranges::find_if(addonInfo, [&it](const AddonInfoPtr& addon)
                               { return it->first == addon->ID(); }) == addonInfo.end();
      if (disabled)
      {
        removed.emplace_back(std::move(it->second));
        it = m_shaderAddons.erase(it);
      }
      else
        ++it;
    }
    std::erase_if(m_failedAddons,
                  [&addonInfo](const auto& item)
                  {
                    return std::ranges::find_if(addonInfo, [&item](const AddonInfoPtr& addon)
                                                { return item.first == addon->ID(); }) ==
                           addonInfo.end();
                  });
  }
  for (auto& shaderAddon : removed)
  {
    RemoveLoader(shaderAddon);
  }

  // Look for new add-ons
  for (const AddonInfoPtr& shaderAddon : addonInfo)
  {
    std::string addonId = shaderAddon->ID();
    if (IsReinstallBlocked(addonId) &&
        ShouldSkipBlockedReinstall(
            addonId, CServiceBroker::GetBinaryAddonManager().GetRunningAddonBase(addonId) !=
                         nullptr))
      continue;
    {
      std::unique_lock lock(m_addonMutex);
      if (m_shaderAddons.contains(addonId) || m_failedAddons.contains(addonId))
        continue;
    }

    auto addonPtr = std::make_shared<CShaderPresetAddon>(shaderAddon);

    if (addonPtr->CreateAddon())
    {
      PublishLoader(addonPtr, addonPtr->GetExtensions());
      std::unique_lock lock(m_addonMutex);
      m_shaderAddons.emplace(std::move(addonId), std::move(addonPtr));
    }
    else
    {
      std::unique_lock lock(m_addonMutex);
      m_failedAddons.try_emplace(std::move(addonId), std::move(addonPtr));
    }
  }
  m_events.Publish(ShaderPresetLoadersChanged{});
}
