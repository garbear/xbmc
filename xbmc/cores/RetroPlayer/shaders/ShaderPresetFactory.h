/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTypes.h"
#include "addons/Addon.h"
#include "utils/EventStream.h"

#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ADDON
{
class CAddonMgr;
class CBinaryAddonManager;
class CShaderPresetAddon;
} // namespace ADDON

namespace KODI::SHADER
{
class CShaderCompileHandle;
class CShaderCompileService;
class IShaderPresetLoader;
class CShaderPresetFactoryTestAccess;

namespace INTERNAL
{
class CShaderPresetLoaderSlot;
struct ShaderPresetLoaderRegistry;
struct ShaderWarmupState;
} // namespace INTERNAL

class CShaderPresetFactory
{
public:
  /*!
   * \brief Create the factory and register all shader preset add-ons
   */
  explicit CShaderPresetFactory(ADDON::CAddonMgr& addons);
  ~CShaderPresetFactory();

  /*!
   * \brief Check if any shader preset add-ons have been loaded
   *
   * This includes add-ons in a failed state.
   *
   * \return True if any shader preset add-ons are present, false otherwise
   */
  bool HasAddons() const;

  /*!
   * \brief Load a preset from the given path
   *
   * \param presetPath The path to the shader preset
   * \param[out] definition The loaded data-only preset definition
   *
   * \return True if the preset was loaded, false otherwise
   */
  bool LoadPreset(std::string_view presetPath, ShaderPresetDefinition& definition) const;

  /*!
   * \brief Check if a registered loader can load a given preset
   *
   * \param presetPath The path to the shader preset
   *
   * \return True if a loader can load the preset, false otherwise
   */
  bool CanLoadPreset(const std::string& presetPath) const;

  CShaderCompileService& CompileService();
  void WarmupPresets(std::string backendId, std::vector<std::string> presetPaths);
  CEventStream<ShaderPresetLoadersChanged>& Events() { return m_events; }

private:
  CShaderPresetFactory();
  void UpdateAddons(std::string_view reinstallId = {});
  void PublishLoader(const std::shared_ptr<IShaderPresetLoader>& loader,
                     const std::vector<std::string>& extensions);
  void ReplaceLoader(const std::shared_ptr<IShaderPresetLoader>& oldLoader,
                     const std::shared_ptr<IShaderPresetLoader>& newLoader,
                     const std::vector<std::string>& extensions);
  void RemoveLoader(const std::shared_ptr<IShaderPresetLoader>& loader);
  void SetWarmupSummaryCallback(std::function<void(const ShaderWarmupSummary&)> callback);
  void SetWarmupRequestCallback(
      std::function<void(const std::shared_ptr<CShaderCompileHandle>&)> callback);
  void BlockReinstall(std::string addonId);
  void ClearReinstallBlock(std::string_view addonId);
  bool IsReinstallBlocked(std::string_view addonId) const;
  bool ShouldSkipBlockedReinstall(std::string_view addonId, bool oldGenerationActive);

  // Construction parameters
  ADDON::CAddonMgr* m_addons{nullptr};
  std::shared_ptr<CShaderCompileService> m_compileService;
  std::shared_ptr<INTERNAL::ShaderPresetLoaderRegistry> m_registry;
  std::shared_ptr<INTERNAL::ShaderWarmupState> m_warmup;

  mutable std::mutex m_addonMutex;
  std::mutex m_reconcileMutex;
  std::map<std::string, std::shared_ptr<ADDON::CShaderPresetAddon>, std::less<>> m_shaderAddons;
  std::map<std::string, std::shared_ptr<ADDON::CShaderPresetAddon>, std::less<>> m_failedAddons;
  std::set<std::string, std::less<>> m_blockedReinstallIds;
  CEventSource<ShaderPresetLoadersChanged> m_events;
  friend class CShaderPresetFactoryTestAccess;
};
} // namespace KODI::SHADER
