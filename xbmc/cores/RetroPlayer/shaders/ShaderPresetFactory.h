/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IShaderPreset.h"
#include "addons/Addon.h"

#include <map>
#include <string>

namespace ADDON
{
  struct AddonEvent;
  class CAddonMgr;
  class CBinaryAddonManager;
  class CShaderPresetAddon;
}

namespace KODI
{
namespace SHADER
{
  class IShaderPresetLoader
  {
  public:
    virtual ~IShaderPresetLoader() = default;

    virtual bool LoadPreset(const std::string &presetPath, IShaderPreset &shaderPreset) = 0;
  };

  class CShaderPresetFactory
  {
  public:

    /*!
     * \brief Create the factory and register all shader preset add-ons
     */
    CShaderPresetFactory(ADDON::CAddonMgr &addons, ADDON::CBinaryAddonManager &binaryAddons);
    ~CShaderPresetFactory();

    void RegisterLoader(IShaderPresetLoader *loader, const std::string &extension);
    void UnregisterLoader(IShaderPresetLoader *loader);

    bool LoadPreset(const std::string &presetPath, IShaderPreset &shaderPreset);
    bool CanLoadPreset(const std::string &presetPath);

  private:
    void OnEvent(const ADDON::AddonEvent &event);
    void UpdateAddons();

    // Construction parameters
    ADDON::CAddonMgr &m_addons;
    ADDON::CBinaryAddonManager &m_binaryAddons;

    std::map<std::string, IShaderPresetLoader*> m_loaders;
    std::vector<std::unique_ptr<ADDON::CShaderPresetAddon>> m_shaderAddons;
    std::vector<std::unique_ptr<ADDON::CShaderPresetAddon>> m_failedAddons;
  };
}
}
