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
#include "FileItem.h"
#include "games/addons/GameClient.h"
#include "threads/SingleLock.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "ServiceBroker.h"

using namespace ADDON;
using namespace GAME;
using namespace XFILE;

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
    if (gc)
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

void CGameManager::GetGameClients(const CFileItem& file, GameClientVector& candidates) const
{
  CSingleLock lock(m_critSection);

  const std::string strRequestedClient = file.GetProperty(FILEITEM_PROPERTY_GAME_CLIENT).asString();
  
  for (GameClientMap::const_iterator it = m_gameClients.begin(); it != m_gameClients.end(); it++)
  {
    const GameClientPtr& gameClient = it->second;
    CLog::Log(LOGDEBUG, "GameManager: Testing %s", gameClient->ID().c_str());
    if (it->second->CanOpen(file))
    {
      CLog::Log(LOGDEBUG, "GameManager: Adding client %s as a candidate", gameClient->ID().c_str());
      candidates.push_back(gameClient);
    }
  }
}

void CGameManager::GetExtensions(std::vector<std::string> &exts) const
{
  CSingleLock lock(m_critSection);
  exts.insert(exts.end(), m_gameExtensions.begin(), m_gameExtensions.end());
}

bool CGameManager::IsGame(const std::string &path) const
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
