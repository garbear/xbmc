#pragma once

#include "cores/RetroPlayer/shaders/IShaderCompiler.h"
#include "cores/RetroPlayer/shaders/IShaderPresetLoader.h"
#include "cores/RetroPlayer/shaders/ShaderCompileService.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"

namespace KODI::SHADER
{
class CShaderPresetFactoryTestAccess
{
public:
  static std::unique_ptr<CShaderPresetFactory> Create()
  {
    return std::unique_ptr<CShaderPresetFactory>(new CShaderPresetFactory());
  }

  static void Publish(CShaderPresetFactory& factory,
                      const std::shared_ptr<IShaderPresetLoader>& loader,
                      std::vector<std::string> extensions)
  {
    factory.PublishLoader(loader, extensions);
  }

  static void Replace(CShaderPresetFactory& factory,
                      const std::shared_ptr<IShaderPresetLoader>& oldLoader,
                      const std::shared_ptr<IShaderPresetLoader>& newLoader,
                      std::vector<std::string> extensions)
  {
    factory.ReplaceLoader(oldLoader, newLoader, extensions);
  }

  static void RegisterCompiler(CShaderPresetFactory& factory,
                               std::shared_ptr<IShaderCompiler> compiler,
                               std::shared_ptr<IShaderArtifactStore> store = {})
  {
    factory.m_compileService->RegisterCompiler(std::move(compiler), std::move(store));
  }

  static void SetSummaryCallback(
      CShaderPresetFactory& factory,
      std::function<void(const ShaderWarmupSummary&)> callback)
  {
    factory.SetWarmupSummaryCallback(std::move(callback));
  }

  static std::weak_ptr<void> WarmupLifetime(CShaderPresetFactory& factory)
  {
    return factory.m_warmup;
  }

  static void SetRequestCallback(
      CShaderPresetFactory& factory,
      std::function<void(const std::shared_ptr<CShaderCompileHandle>&)> callback)
  {
    factory.SetWarmupRequestCallback(std::move(callback));
  }

  static void BlockReinstall(CShaderPresetFactory& factory, std::string addonId)
  {
    factory.BlockReinstall(std::move(addonId));
  }

  static void ClearReinstallBlock(CShaderPresetFactory& factory, std::string_view addonId)
  {
    factory.ClearReinstallBlock(addonId);
  }

  static bool IsReinstallBlocked(const CShaderPresetFactory& factory, std::string_view addonId)
  {
    return factory.IsReinstallBlocked(addonId);
  }

  static bool ShouldSkipBlockedReinstall(CShaderPresetFactory& factory,
                                         std::string_view addonId,
                                         bool oldGenerationActive)
  {
    return factory.ShouldSkipBlockedReinstall(addonId, oldGenerationActive);
  }
};
} // namespace KODI::SHADER
