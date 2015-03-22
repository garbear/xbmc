/*
 *      Copyright (C) 2012-2014 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RetroPlayerDialogs.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogOK.h"
#include "FileItem.h"
#include "games/addons/GameClient.h"
#include "games/GameManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <map>
#include <memory>

using namespace ADDON;
using namespace GAME;

bool CRetroPlayerDialogs::GetGameClient(const CFileItem &file, GameClientPtr &result)
{
  // See how many game clients contend for this file
  std::vector<std::string> candidates;
  CGameManager::Get().GetGameClientIDs(file, candidates);

  if (candidates.empty())
  {
    // No compatible game clients. Enable just-in-time game client installation
    // by asking the user if they would like to download and install a client
    // and then use it to play the game, or go to the add-on manager.
    return GameLauchDialog(file, result);
  }
  else if (candidates.size() == 1)
  {
    // Just use the single compatible game client. More game clients can be
    // downloaded in the add-on manager.
    AddonPtr addon;
    GameClientPtr gc;
    if (CAddonMgr::Get().GetAddon(candidates[0], addon, ADDON_GAMEDLL))
      gc = std::dynamic_pointer_cast<CGameClient>(addon);
    if (!gc)
    {
      CLog::Log(LOGDEBUG, "RetroPlayer: Game client failed");
      // "id"
      // "An unknown error has occurred."
      // "Check the log file for details."
      CGUIDialogOK::ShowAndGetInput(candidates[0].c_str(), 24071, 16029, 0);
      return false;
    }
    result = gc;
  }
  else // candidates.size() > 1
  {
    return ChooseGameClientDialog(candidates, file, result);
  }

  return true;
}

bool CRetroPlayerDialogs::GameLauchDialog(const CFileItem &file, GameClientPtr &result)
{
  CFileItem fileCopy = file;
  // If an explicit game client was specified, try to download that
  if (fileCopy.HasProperty("gameclient"))
  {
    if (InstallGameClient(fileCopy.GetProperty("gameclient").asString(), fileCopy, result))
      return true;
    fileCopy.ClearProperty("gameclient"); // don't want this to interfere later on
  }

  // First, ask the user if they would like to install a game client or go to
  // the add-on manager
  CContextButtons choices;
  choices.Add(0, 27014); // Install emulator
  choices.Add(1, 27015); // Add-on manager

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  if (btnid == 0) // Install emulator
  {
    return InstallGameClientDialog(fileCopy, result);
  }
  else if (btnid == 1) // Add-on manager
  {
    // Queue the file so that if a compatible game client is installed, the
    // user will be asked to launch the file.
    CGameManager::Get().SetAutoLaunch(fileCopy);
    CLog::Log(LOGDEBUG, "RetroPlayer: User chose to go to the add-on manager");
    std::vector<std::string> params;
    params.push_back("addons://all/kodi.gameclient");
    g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
  }
  else
  {
    CLog::Log(LOGDEBUG, "RetroPlayer: User canceled game client selection");
  }

  return false;
}

bool CRetroPlayerDialogs::InstallGameClientDialog(const CFileItem &file, GameClientPtr &result)
{
  VECADDONS addons;
  CGameManager::GetAllGameClients(addons);

  std::map<std::string, GameClientPtr> candidates;
  for (VECADDONS::const_iterator itRemote = addons.begin(); itRemote != addons.end(); ++itRemote)
  {
    if (!(*itRemote)->IsType(ADDON_GAMEDLL))
      continue;
    GameClientPtr gc = std::dynamic_pointer_cast<CGameClient>(*itRemote);

    // Only add game clients to the list if they provide extensions or platforms
    if (!gc || (gc->GetExtensions().empty() /* && gc->GetPlatforms().empty() */))
      continue;

    if (!gc->CanOpen(file))
      continue;

    // If the game client is already installed and enabled, exclude it from the list
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(gc->ID(), addon, ADDON_GAMEDLL, true))
      continue;

    // If GetAddon() returns false, but addon is non-NULL, then the add-on exists but is disabled
    bool bEnabled = (addon.get() == NULL);

    bool bBroken = !(*itRemote)->Props().broken.empty();

    if (bEnabled && bBroken)
      continue;

    std::string strName = gc->Name();

    // Append "(Disabled)" to the name if the add-on is disabled
    if (!bEnabled)
      strName = StringUtils::Format("%s (%s)", strName.c_str(), g_localizeStrings.Get(1223).c_str());

    candidates[strName] = gc;
  }

  if (candidates.empty())
  {
    CLog::Log(LOGDEBUG, "RetroPlayer: No compatible game clients for installation");
    // "Playback failed"
    // "No compatible emulators found for file:"
    // "FILENAME"
    CGUIDialogOK::ShowAndGetInput(16026, 27013, URIUtils::GetFileName(file.GetPath()), 0);
    return false;
  }

  // CContextButtons doesn't support keying by string, only int, so use a
  // parallel array to track the string values (client name)
  CContextButtons choicesInt;
  unsigned int i = 0;

  // Vector to translate button IDs to game client pointers
  std::vector<std::string> choicesStr;
  for (std::map<std::string, GameClientPtr>::const_iterator it = candidates.begin(); it != candidates.end(); ++it)
  {
    const std::string& strName = it->first;
    choicesInt.Add(i++, strName);
    choicesStr.push_back(strName);
  }

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(choicesInt);
  if (btnid < 0 || btnid >= (int)candidates.size())
  {
    CLog::Log(LOGDEBUG, "RetroPlayer: User canceled game client installation selection");
    return false;
  }

  std::map<std::string, GameClientPtr>::iterator it = candidates.find(choicesStr[btnid]);
  if (it == candidates.end())
    return false; // Shouldn't happen

  GameClientPtr &gc = it->second;

  // determining disabled status is the same test as earlier
  // TODO: Preserve disabled values in parallel array
  // TODO: Don't use parallel arrays
  AddonPtr addon; // Return value, will be assigned to result
  bool disabled = !CAddonMgr::Get().GetAddon(gc->ID(), addon, ADDON_GAMEDLL, true) && addon;

  if (disabled)
  {
    // TODO: Prompt the user to enable it
    CLog::Log(LOGDEBUG, "RetroPlayer: Game client %s installed but disabled, enabling it", gc->ID().c_str());
    CGameManager::Get().ClearAutoLaunch(); // Don't auto-launch queued game when the add-on is enabled
    CAddonMgr::Get().DisableAddon(gc->ID(), false);

    // Retrieve the newly-enabled add-on
    if (!CAddonMgr::Get().GetAddon(gc->ID(), addon) || !addon)
      return false;
  }
  else
  {
    CLog::Log(LOGDEBUG, "RetroPlayer: Installing game client %s", gc->ID().c_str());
    bool success = false; // CAddonInstaller::Get().PromptForInstall(gc->ID(), addon); // TODO
    if (!success || !addon || addon->Type() != ADDON_GAMEDLL)
    {
      CLog::Log(LOGDEBUG, "RetroPlayer: Game client installation failed");
      // "id"
      // "Installation failed"
      // "Check the log file for details."
      CGUIDialogOK::ShowAndGetInput(gc->ID().c_str(), 114, 16029, 0);
      return false;
    }
  }

  GameClientPtr gameClient = std::dynamic_pointer_cast<CGameClient>(addon);
  if (!gameClient)
  {
    CLog::Log(LOGERROR, "RetroPlayer: Add-on was not a game client!");
    return false;
  }

  result = gameClient;
  return true;
}

bool CRetroPlayerDialogs::InstallGameClient(const std::string &strId, const CFileItem &file, GameClientPtr &result)
{
  // First, make sure the game client isn't installed
  CLog::Log(LOGDEBUG, "RetroPlayer: Trying to install game client %s", strId.c_str());
  AddonPtr addon;
  bool installed = CAddonMgr::Get().GetAddon(strId, addon, ADDON_GAMEDLL, false);
  if (installed && addon)
    return false;

  // Now make sure it exists in a remote repository
  CAddonDatabase database;
  if (database.Open() && database.GetAddon(strId, addon))
  {
    GameClientPtr gc = std::dynamic_pointer_cast<CGameClient>(addon);
    if (gc && gc->CanOpen(file))
    {
      CLog::Log(LOGDEBUG, "RetroPlayer: Installing game client %s", strId.c_str());
      addon.reset();
      /* TODO
      if (CAddonInstaller::Get().PromptForInstall(strId, addon))
      {
        gc = std::dynamic_pointer_cast<CGameClient>(addon);
        if (gc)
        {
          result = gc;
          return true;
        }
      }
      */
    }
  }
  return false;
}

bool CRetroPlayerDialogs::ChooseGameClientDialog(const std::vector<std::string> &clientIds, const CFileItem &file, GameClientPtr &result)
{
  CLog::Log(LOGDEBUG, "RetroPlayer: Multiple clients found: %s", StringUtils::Join(clientIds, ", ").c_str());

  // Turn ID strings into game client pointers (std::map enables sorting by name)
  std::map<std::string, GameClientPtr> clients;
  for (std::vector<std::string>::const_iterator it = clientIds.begin(); it != clientIds.end(); ++it)
  {
    AddonPtr addon;
    GameClientPtr gc;
    if (CAddonMgr::Get().GetAddon(*it, addon, ADDON_GAMEDLL))
    {
      gc = std::dynamic_pointer_cast<CGameClient>(addon);
      if (gc)
      {
        std::string strName = gc->Name();
        // Make lower case for sorting purposes
        StringUtils::ToLower(strName);
        clients[strName] = gc;
      }
    }
  }

  // CContextButtons doesn't support keying by string, only int, so use a
  // parallel array to track the string values (client name)
  CContextButtons choicesInt;
  unsigned int i = 0;

  std::vector<std::string> choicesStr;
  choicesStr.reserve(clients.size());

  for (std::map<std::string, GameClientPtr>::const_iterator it = clients.begin(); it != clients.end(); ++it)
  {
    std::string strName = it->second->Name();
    choicesInt.Add(i++, strName);
    // Remember, our map keys are lower case
    StringUtils::ToLower(strName);
    choicesStr.push_back(strName);
  }

  // i becomes the index of the final item (choice to go to the add-on manager)
  const unsigned int iAddonMgr = i;
  choicesInt.Add(iAddonMgr, 27005); // "Manage emulators..."

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(choicesInt);
  if (btnid < 0 || btnid > (int)iAddonMgr)
  {
    CLog::Log(LOGDEBUG, "RetroPlayer: User cancelled game client selection");
    return false;
  }
  else if (btnid == (int)iAddonMgr)
  {
    // Queue the file so that if a compatible game client is installed, the
    // user will be asked to launch the file.
    CGameManager::Get().SetAutoLaunch(file);

    CLog::Log(LOGDEBUG, "RetroPlayer: User chose to go to the add-on manager");
    std::vector<std::string> params;
    params.push_back("addons://all/kodi.gameclient");
    g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
    return false;
  }
  else
  {
    result = clients[choicesStr[btnid]];
    CLog::Log(LOGDEBUG, "RetroPlayer: Using %s", result->ID().c_str());
  }

  return true;
}
