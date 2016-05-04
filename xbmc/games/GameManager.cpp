/*
 *      Copyright (C) 2012-2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameManager.h"
#include "addons/Addon.h"
#include "dialogs/GUIDialogOK.h"
#include "games/addons/GameClient.h"
#include "games/dialogs/GUIDialogSelectGameClient.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"

#include <algorithm>

using namespace ADDON;
using namespace GAME;

CGameManager& CGameManager::GetInstance()
{
  return CServiceBroker::GetGameManager();
}

void CGameManager::Start()
{
  UpdateAddons();

  CAddonMgr::GetInstance().RegisterObserver(this);
  CAddonMgr::GetInstance().RegisterAddonMgrCallback(ADDON_GAMEDLL, this);
}

void CGameManager::Stop()
{
  CAddonMgr::GetInstance().UnregisterObserver(this);
  CAddonMgr::GetInstance().UnregisterAddonMgrCallback(ADDON_GAMEDLL);
}

void CGameManager::UpdateAddons()
{
  VECADDONS addonsEnabled;
  VECADDONS addonsAll;

  CAddonMgr::GetInstance().GetAddons(addonsEnabled, ADDON_GAMEDLL);
  CAddonMgr::GetInstance().GetInstalledAddons(addonsAll, ADDON_GAMEDLL);
  CAddonMgr::GetInstance().GetInstallableAddons(addonsAll, ADDON_GAMEDLL);

  CSingleLock lock(m_critSection);

  // Process add-ons
  bool bModified = false;
  for (const AddonPtr& addon : addonsEnabled)
  {
    GameClientPtr gameClient = std::dynamic_pointer_cast<CGameClient>(addon);
    if (!gameClient)
      continue;

    if (m_gameClients.find(gameClient->ID()) != m_gameClients.end())
      continue; // Already registered

    m_gameClients[gameClient->ID()] = gameClient;
    // TODO: Create directories
    bModified = true;

    CLog::Log(LOGDEBUG, "GameManager: Registered add-on %s", gameClient->ID().c_str());
  }

  // Process extensions
  for (const AddonPtr& addon : addonsAll)
  {
    GameClientPtr gc = std::dynamic_pointer_cast<CGameClient>(addon);
    if (!gc)
      continue;

    const bool bWildcard = (gc->GetExtensions().find("*") != gc->GetExtensions().end());
    if (!bWildcard)
      m_gameExtensions.insert(gc->GetExtensions().begin(), gc->GetExtensions().end());
  }

  CLog::Log(LOGDEBUG, "GameManager: tracking %u extensions", (unsigned)m_gameExtensions.size());
}

bool CGameManager::GetAddonInstance(const std::string& strClientId, GameClientPtr& addon) const
{
  CSingleLock lock(m_critSection);

  auto it = m_gameClients.find(strClientId);
  if (it != m_gameClients.end())
  {
    addon = it->second;
    return true;
  }
  return false;
}

GameClientPtr CGameManager::OpenGameClient(const CFileItem& file)
{
  GameClientPtr gameClient;

  // Get the game client ID from the file properties
  std::string gameClientId = file.GetProperty(FILEITEM_PROPERTY_GAME_CLIENT).asString();

  // If the fileitem's add-on is a game client, fall back to that
  if (gameClientId.empty())
  {
    if (file.HasAddonInfo() && file.GetAddonInfo()->Type() == ADDON::ADDON_GAMEDLL)
      gameClientId = file.GetAddonInfo()->ID();
  }

  // Resolve ID to game client ptr
  if (!gameClientId.empty())
  {
    ADDON::AddonPtr addon;
    if (ADDON::CAddonMgr::GetInstance().GetAddon(gameClientId, addon, ADDON::ADDON_GAMEDLL))
      gameClient = std::dynamic_pointer_cast<GAME::CGameClient>(addon);
  }

  // Need to prompt the user
  if (!gameClient)
  {
    GameClientVector candidates;
    GameClientVector installable;
    GetGameClients(file, candidates, installable);

    if (candidates.empty() && installable.empty())
    {
      // "Failed to play game"
      // "This game isn't compatible with any available emulators."
      CGUIDialogOK::ShowAndGetInput(CVariant{ 35210 }, CVariant{ 35212 });
    }
    else if (candidates.size() == 1 && installable.empty())
    {
      // Only 1 option, avoid prompting the user
      gameClient = candidates[0];
    }
    else
    {
      CGUIDialogSelectGameClient::ShowAndGetGameClient(candidates, installable, gameClient);
    }
  }

  return gameClient;
}

void CGameManager::GetGameClients(const CFileItem& file, GameClientVector& candidates, GameClientVector& installable) const
{
  // Get local candidates
  {
    CSingleLock lock(m_critSection);
    for (GameClientMap::const_iterator it = m_gameClients.begin(); it != m_gameClients.end(); it++)
    {
      const GameClientPtr& gameClient = it->second;
      if (it->second->CanOpen(file))
        candidates.push_back(gameClient);
    }
  }

  // Get remote candidates
  VECADDONS addons;
  if (CAddonMgr::GetInstance().GetInstallableAddons(addons, ADDON_GAMEDLL))
  {
    for (auto& addon : addons)
    {
      GameClientPtr gameClient = std::dynamic_pointer_cast<CGameClient>(addon);
      if (!gameClient)
        continue;

      if (gameClient->CanOpen(file))
        installable.push_back(gameClient);
    }
  }

  // Sort by name (TODO: Move to presentation code)
  auto SortByName = [](const GameClientPtr& lhs, const GameClientPtr& rhs)
    {
      std::string lhsName = lhs->Name();
      std::string rhsName = rhs->Name();

      StringUtils::ToLower(lhsName);
      StringUtils::ToLower(rhsName);

      return lhsName < rhsName;
    };

  std::sort(candidates.begin(), candidates.end(), SortByName);
  std::sort(installable.begin(), installable.end(), SortByName);
}

void CGameManager::GetExtensions(std::vector<std::string> &exts) const
{
  CSingleLock lock(m_critSection);
  exts.insert(exts.end(), m_gameExtensions.begin(), m_gameExtensions.end());
}

bool CGameManager::HasGameExtension(const std::string &path) const
{
  // Get the file extension (must use a CURL, if the string is top-level zip
  // directory it might not end in .zip)
  std::string extension(URIUtils::GetExtension(CURL(path).GetFileNameWithoutPath()));
  if (extension.empty())
    return false;

  StringUtils::ToLower(extension);

  CSingleLock lock(m_critSection);

  return m_gameExtensions.find(extension) != m_gameExtensions.end();
}

bool CGameManager::RequestRemoval(AddonPtr addon)
{
  // TODO: Remove addon from m_gameClients;
  return true;
}

void CGameManager::Notify(const Observable& obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageAddons)
    UpdateAddons();
}
