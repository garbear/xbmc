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

#include "GUIWindowGames.h"
#include "addons/AddonInstaller.h" // TODO
#include "addons/IAddon.h" // TODO
#include "addons/GUIDialogAddonInfo.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "Application.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "dialogs/GUIDialogProgress.h"
#include "FileItem.h"
#include "games/tags/GameInfoTag.h"
#include "games/addons/GameClient.h"
#include "games/GameManager.h"
#include "games/GameTypes.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "GUIPassword.h"
#include "input/Key.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "URL.h"
#include "Util.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <iterator>

using namespace GAME;

CGUIWindowGames::CGUIWindowGames() :
  CGUIMediaWindow(WINDOW_GAMES, "MyGames.xml"),
  m_dlgProgress(nullptr)
{
}

bool CGUIWindowGames::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_rootDir.AllowNonLocalSources(true); // TODO

      // Is this the first time the window is opened?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        message.SetStringParam(CMediaSourceSettings::GetInstance().GetDefaultSource("games"));

      // TODO
      m_dlgProgress = dynamic_cast<CGUIDialogProgress*>(g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS));

      break;
    }
    case GUI_MSG_CLICKED:
    {
      if (OnClickMsg(message.GetSenderId(), message.GetParam1()))
        return true;
      break;
    }
    default:
      break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowGames::OnClickMsg(int controlId, int actionId)
{
  if (!m_viewControl.HasControl(controlId))  // list/thumb control
    return false;

  const int iItem = m_viewControl.GetSelectedItem();

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  if (!pItem)
    return false;

  switch (actionId)
  {
  case ACTION_DELETE_ITEM:
  {
    // Is delete allowed?
    if (CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_ALLOWFILEDELETION))
    {
      OnDeleteItem(iItem);
      return true;
    }
    break;
  }
  case ACTION_PLAYER_PLAY:
  {
    if (OnClick(iItem))
      return true;
    break;
  }
  case ACTION_SHOW_INFO:
  {
    if (!m_vecItems->IsPlugin())
    {
      if (pItem->HasAddonInfo())
      {
        CGUIDialogAddonInfo::ShowForItem(pItem);
        return true;
      }
    }
    break;
  }
  default:
    break;
  }

  return false;
}

void CGUIWindowGames::SetupShares()
{
  CGUIMediaWindow::SetupShares();
  // XBMC downloads a list of supported extensions from the remote add-ons
  // repo. Zip files are treated as directories and scanned recursively; if
  // they don't contain valid extensions (such as MAME arcade games), the
  // entire zip will be missing from the MyGames window. Skipping the recursive
  // scan always shows zip files (note: entering the zip will show an empty
  // folder) and speeds up directory listing as a nice side effect.
  //m_rootDir.SetFlags(XFILE::DIR_FLAG_NO_FILE_DIRS);
}

bool CGUIWindowGames::OnClick(int iItem, const std::string &player /* = "" */)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (item && !item->m_bIsFolder)
  {
    PlayGame(*item);
    return true;
  }

  return CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowGames::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);

  if (item && !item->GetProperty("pluginreplacecontextitems").asBoolean())
  {
    if (m_vecItems->IsSourcesPath())
    {
      // Context buttons for a sources path, like "Add source", "Remove Source", etc.
      CGUIDialogContextMenu::GetContextButtons("games", item, buttons);
    }
    else if (m_vecItems->IsVirtualDirectoryRoot())
    {
      // TODO
    }
    else
    {
      if (item->IsGame())
        buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208); // Play

      if (!m_vecItems->IsPlugin() && item->HasAddonInfo())
        buttons.Add(CONTEXT_BUTTON_INFO, 24003); // Add-on information

      if (CSettings::GetInstance().GetBool("filelists.allowfiledeletion") && !item->IsReadOnly())
      {
        buttons.Add(CONTEXT_BUTTON_DELETE, 117);
        buttons.Add(CONTEXT_BUTTON_RENAME, 118);
      }
    }
  }

  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowGames::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (item)
  {
    if (m_vecItems->IsSourcesPath())
    {
      if (CGUIDialogContextMenu::OnContextButton("games", item, button))
      {
        Update("sources://games/");
        return true;
      }
    }
    switch (button)
    {
    case CONTEXT_BUTTON_PLAY_ITEM:
      PlayGame(*item);
      return true;
    case CONTEXT_BUTTON_INFO:
      CGUIDialogAddonInfo::ShowForItem(item);
      return true;
    case CONTEXT_BUTTON_DELETE:
      OnDeleteItem(itemNumber);
      return true;
    case CONTEXT_BUTTON_RENAME:
      OnRenameItem(itemNumber);
      return true;
    default:
      break;
    }
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowGames::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource("games");
}

bool CGUIWindowGames::GetDirectory(const std::string &strDirectory, CFileItemList& items)
{
  if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
    return false;

  // Set label
  std::string label;
  if (items.GetLabel().empty())
    m_rootDir.IsSource(items.GetPath(), CMediaSourceSettings::GetInstance().GetSources("games"), &label);

  if (!label.empty())
    items.SetLabel(label);

  // Set content
  std::string content;
  if (items.GetContent().empty())
  {
    if (!items.IsVirtualDirectoryRoot() && // Don't set content for root directory
        !items.IsPlugin())                 // Don't set content for plugins
    {
      content = "games";
    }
  }

  if (!content.empty())
    items.SetContent(content);

  return true;
}

std::string CGUIWindowGames::GetStartFolder(const std::string &dir)
{
  // From CGUIWindowPictures::GetStartFolder()

  if (StringUtils::EqualsNoCase(dir, "plugins") ||
      StringUtils::EqualsNoCase(dir, "addons"))
  {
    return "addons://sources/game/";
  }

  SetupShares();
  VECSOURCES shares;
  m_rootDir.GetSources(shares);
  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(dir, shares, bIsSourceName);
  if (iIndex >= 0)
  {
    if (iIndex < (int)shares.size() && shares[iIndex].m_iHasLock == 2)
    {
      CFileItem item(shares[iIndex]);
      if (!g_passwordManager.IsItemUnlocked(&item, "games"))
        return "";
    }
    if (bIsSourceName)
      return shares[iIndex].strPath;
    return dir;
  }
  return CGUIMediaWindow::GetStartFolder(dir);
}

void CGUIWindowGames::OnItemInfo(int itemNumber)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;

  if (!m_vecItems->IsPlugin())
  {
    if (item->IsPlugin() || item->IsScript())
      CGUIDialogAddonInfo::ShowForItem(item);
  }

  /* TODO
  CGUIDialogGameInfo* gameInfo = dynamic_cast<CGUIDialogGameInfo*>(g_windowManager.GetWindow(WINDOW_DIALOG_PICTURE_INFO));
  if (gameInfo)
  {
    gameInfo->SetGame(item);
    gameInfo->Open();
  }
  */
}

bool CGUIWindowGames::PlayGame(const CFileItem &item)
{
  using namespace ADDON;

  // Add-ons must be of game client type
  if (item.HasAddonInfo() && item.GetAddonInfo()->Type() != ADDON_GAMEDLL)
    return false;

  // Get the game client ID from the file properties
  std::string requestedClient = item.GetProperty(FILEITEM_PROPERTY_GAME_CLIENT).asString();

  // Get the game client ID from the add-on info
  if (requestedClient.empty())
  {
    if (item.HasAddonInfo())
      requestedClient = item.GetAddonInfo()->ID();
  }

  // Ask the user for a game add-on
  if (requestedClient.empty())
    requestedClient = GetGameClient(item);

  if (!requestedClient.empty())
  {
    CFileItem gameFile = item;

    /* TODO
    // Decode zip root directory (zip://%2Fpath_to_file.zip/ -> file:///path_to_file.zip)
    CURL url(item.GetPath());
    if (url.GetProtocol() == "zip" && url.GetFileName() == "")
      gameFile.SetPath(url.GetHostName());
    */

    gameFile.SetProperty(FILEITEM_PROPERTY_GAME_CLIENT, requestedClient);

    return g_application.PlayFile(gameFile, "") == PLAYBACK_OK;
  }

  return true;
}

std::string CGUIWindowGames::GetGameClient(const CFileItem &item)
{
  std::string requestedClient;

  // Ask Game Manager
  GameClientVector gameClients;
  CGameManager::GetInstance().GetGameClients(item, gameClients);

  if (gameClients.empty())
  {
    // Ask the user to download and install a client
    requestedClient = InstallByChoice();
  }
  else if (gameClients.size() == 1)
  {
    // Avoid prompting the user if exactly one game client was found
    requestedClient = gameClients[0]->ID();
  }
  else
  {
    // Ask the user to choose an emulator
    requestedClient = ChooseGameClient(gameClients);
  }

  return requestedClient;
}

std::string CGUIWindowGames::InstallByChoice()
{
  using namespace ADDON;

  std::string chosenClientId;

  // First, ask the user if they would like to install a game client or go to
  // the add-on manager
  CContextButtons choices;
  choices.Add(0, 35253); // Install emulator
  choices.Add(1, 35254); // Manage emulators

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  if (btnid == 0) // Install emulator
  {
    if (CGUIWindowAddonBrowser::SelectAddonID(ADDON::ADDON_GAMEDLL, chosenClientId, false, true, false, true, false) >= 0)
    {
      if (!chosenClientId.empty())
        CLog::Log(LOGDEBUG, "RetroPlayer: User installed %s", chosenClientId.c_str());
    }
  }
  else if (btnid == 1) // Manage emulators
  {
    ActivateAddonMgr();
  }
  else
  {
    CLog::Log(LOGDEBUG, "RetroPlayer: User canceled emulator selection");
  }

  return chosenClientId;
}

std::string CGUIWindowGames::ChooseGameClient(const GameClientVector& gameClients)
{
  std::string chosenClientId;

  // Log clients
  std::vector<std::string> clientIds;
  std::transform(gameClients.begin(), gameClients.end(), std::back_inserter(clientIds),
    [](const GameClientPtr& client)
    {
      return client->ID();
    });
  CLog::Log(LOGDEBUG, "RetroPlayer: Multiple clients found: %s", StringUtils::Join(clientIds, ", ").c_str());

  CContextButtons choiceButtons;

  // Add emulators
  int i = 0;
  for (const GameClientPtr& gameClient : gameClients)
    choiceButtons.Add(i++, gameClient->Name());

  // Add button to manage emulators
  const int iAddonMgr = i;
  choiceButtons.Add(i++, 35254); // "Manage emulators"

  // Do modal
  int result = CGUIDialogContextMenu::ShowAndGetChoice(choiceButtons);

  if (0 <= result && result < static_cast<int>(gameClients.size()))
  {
    chosenClientId = gameClients[result]->ID();
  }
  else if (result == iAddonMgr)
  {
    ActivateAddonMgr();
  }
  else
  {
    CLog::Log(LOGDEBUG, "RetroPlayer: User cancelled game client selection");
  }

  return chosenClientId;
}

void CGUIWindowGames::ActivateAddonMgr()
{
  CLog::Log(LOGDEBUG, "RetroPlayer: User chose to go to the add-on manager");
  std::vector<std::string> params;
  params.push_back("addons://user/category.emulators");
  g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
}
