/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetFactory.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "addons/binary-addons/BinaryAddonManager.h"
#include "addons/AddonManager.h"
#include "addons/ShaderPreset.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <string>

using namespace KODI;
using namespace SHADER;

CShaderPresetFactory::CShaderPresetFactory(ADDON::CAddonMgr &addons, ADDON::CBinaryAddonManager &binaryAddons) :
  m_addons(addons),
  m_binaryAddons(binaryAddons)
{
  UpdateAddons();

  m_addons.Events().Subscribe(this, &CShaderPresetFactory::OnEvent);
}

CShaderPresetFactory::~CShaderPresetFactory()
{
  m_addons.Events().Unsubscribe(this);
}

void CShaderPresetFactory::RegisterLoader(IShaderPresetLoader *loader, const std::string &extension)
{
  if (!extension.empty())
  {
    std::string strExtension = extension;

    // Canonicalize extension with leading "."
    if (extension[0] != '.')
      strExtension.insert(strExtension.begin(), '.');

    m_loaders.insert(std::make_pair(std::move(strExtension), loader));
  }
}

void CShaderPresetFactory::UnregisterLoader(IShaderPresetLoader *loader)
{
  for (auto it = m_loaders.begin(); it != m_loaders.end(); )
  {
    if (it->second == loader)
      m_loaders.erase(it++);
    else
      ++it;
  }
}

bool CShaderPresetFactory::LoadPreset(const std::string &presetPath, IShaderPreset &shaderPreset)
{
  bool bSuccess = false;

  std::string extension = URIUtils::GetExtension(presetPath);
  if (!extension.empty())
  {
    auto itLoader = m_loaders.find(extension);
    if (itLoader != m_loaders.end())
      bSuccess = itLoader->second->LoadPreset(presetPath, shaderPreset);
  }

  return bSuccess;
}

void CShaderPresetFactory::OnEvent(const ADDON::AddonEvent &event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
      typeid(event) == typeid(ADDON::AddonEvents::UnInstalled)) //! @todo Other events?
    UpdateAddons();
}

void CShaderPresetFactory::UpdateAddons()
{
  using namespace ADDON;

  BinaryAddonBaseList addonInfos;
  m_binaryAddons.GetAddonInfos(addonInfos, true, ADDON_SHADERDLL);

  // Look for removed/disabled add-ons
  auto oldAddons = std::move(m_shaderAddons);
  for (auto &shaderAddon : oldAddons)
  {
    const bool bIsDisabled = std::find_if(addonInfos.begin(), addonInfos.end(),
      [&shaderAddon](const BinaryAddonBasePtr &addon)
      {
        return shaderAddon->ID() == addon->ID();
      }) == addonInfos.end();

    if (bIsDisabled)
      UnregisterLoader(shaderAddon.get());
    else
      m_shaderAddons.emplace_back(std::move(shaderAddon));
  }

  // Look for new add-ons
  for (const auto &shaderAddon : addonInfos)
  {
    auto FindAddonById = [&shaderAddon](const std::unique_ptr<CShaderPresetAddon> &addon)
      {
        return shaderAddon->ID() == addon->ID();
      };

    const bool bIsNew = std::find_if(m_shaderAddons.begin(), m_shaderAddons.end(), FindAddonById) == m_shaderAddons.end();

    if (!bIsNew)
      continue;

    const bool bIsFailed = std::find_if(m_failedAddons.begin(), m_failedAddons.end(), FindAddonById) != m_failedAddons.end();

    if (bIsFailed)
      continue;

    std::unique_ptr<CShaderPresetAddon> addonPtr(new CShaderPresetAddon(shaderAddon));
    if (addonPtr->CreateAddon())
    {
      for (const auto &extension : addonPtr->GetExtensions())
        RegisterLoader(addonPtr.get(), extension);
      m_shaderAddons.emplace_back(std::move(addonPtr));
    }
    else
    {
      m_failedAddons.emplace_back(std::move(addonPtr));
    }
  }
}

bool CShaderPresetFactory::CanLoadPreset(const std::string &presetPath)
{
  bool bSuccess = false;

  std::string extension = URIUtils::GetExtension(presetPath);
  if (!extension.empty())
  {
    auto itLoader = m_loaders.find(extension);
    bSuccess = itLoader != m_loaders.end();
  }

  return bSuccess;
}
