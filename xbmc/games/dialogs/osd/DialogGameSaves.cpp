/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameSaves.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroEngine/RetroEngine.h"
#include "cores/RetroEngine/streams/RetroEngineStreamManager.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/addons/GameClient.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/Key.h"
#include "utils/FileUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

CDialogGameSaves::CDialogGameSaves()
  : CGUIDialog(WINDOW_DIALOG_GAME_SAVES, "DialogSelect.xml"),
    m_viewControl(std::make_unique<CGUIViewControl>()),
    m_vecList(std::make_unique<CFileItemList>())
{
}

bool CDialogGameSaves::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      const int actionId = message.GetParam1();

      switch (actionId)
      {
        case ACTION_SELECT_ITEM:
        case ACTION_MOUSE_LEFT_CLICK:
        {
          int selectedId = m_viewControl->GetSelectedItem();
          if (0 <= selectedId && selectedId < m_vecList->Size())
          {
            CFileItemPtr item = m_vecList->Get(selectedId);
            if (item)
            {
              for (int i = 0; i < m_vecList->Size(); i++)
                m_vecList->Get(i)->Select(false);

              item->Select(true);

              OnSelect(*item);

              return true;
            }
          }
          break;
        }
        case ACTION_CONTEXT_MENU:
        case ACTION_MOUSE_RIGHT_CLICK:
        {
          int selectedItem = m_viewControl->GetSelectedItem();
          if (selectedItem >= 0 && selectedItem < m_vecList->Size())
          {
            CFileItemPtr item = m_vecList->Get(selectedItem);
            if (item)
            {
              OnContextMenu(*item);
              return true;
            }
          }
          break;
        }
        case ACTION_RENAME_ITEM:
        {
          const int controlId = message.GetSenderId();
          if (m_viewControl->HasControl(controlId))
          {
            int selectedItem = m_viewControl->GetSelectedItem();
            if (selectedItem >= 0 && selectedItem < m_vecList->Size())
            {
              CFileItemPtr item = m_vecList->Get(selectedItem);
              if (item)
              {
                OnRename(*item);
                return true;
              }
            }
          }
          break;
        }
        case ACTION_DELETE_ITEM:
        {
          const int controlId = message.GetSenderId();
          if (m_viewControl->HasControl(controlId))
          {
            int selectedItem = m_viewControl->GetSelectedItem();
            if (selectedItem >= 0 && selectedItem < m_vecList->Size())
            {
              CFileItemPtr item = m_vecList->Get(selectedItem);
              if (item)
              {
                OnDelete(*item);
                return true;
              }
            }
          }
          break;
        }
        default:
          break;
      }

      const int controlId = message.GetSenderId();
      switch (controlId)
      {
        case CONTROL_SAVES_NEW_BUTTON:
        {
          m_bNewPressed = true;
          Close();
          break;
        }
        case CONTROL_SAVES_CANCEL_BUTTON:
        {
          m_selectedItem.reset();
          m_vecList->Clear();
          m_bConfirmed = false;
          Close();
          break;
        }
        default:
          break;
      }

      break;
    }

    case GUI_MSG_SETFOCUS:
    {
      const int controlId = message.GetControlId();
      if (m_viewControl->HasControl(controlId))
      {
        if (m_vecList->IsEmpty())
        {
          SET_CONTROL_FOCUS(CONTROL_SAVES_NEW_BUTTON, 0);
          return true;
        }

        if (m_viewControl->GetCurrentControl() != controlId)
        {
          m_viewControl->SetFocused();
          return true;
        }
      }
      break;
    }

    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

void CDialogGameSaves::FrameMove()
{
  CGUIControl* itemContainer = GetControl(CONTROL_SAVES_DETAILED_LIST);
  if (itemContainer != nullptr)
  {
    if (itemContainer->HasFocus())
    {
      int selectedItem = m_viewControl->GetSelectedItem();
      if (selectedItem >= 0 && selectedItem < m_vecList->Size())
      {
        CFileItemPtr item = m_vecList->Get(selectedItem);
        if (item)
          OnFocus(*item);
      }
    }
    else
    {
      OnFocusLost();
    }
  }

  CGUIDialog::FrameMove();
}

void CDialogGameSaves::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Initialize view control
  m_viewControl->SetCurrentView(CONTROL_SAVES_DETAILED_LIST);

  // Select the first item
  m_viewControl->SetSelectedItem(0);

  // Get game clients for all savestates
  for (auto it = m_vecList->cbegin(); it != m_vecList->cend(); ++it)
  {
    const CFileItem& item = **it;
    std::string gameClientId = item.GetProperty(SAVESTATE_GAME_CLIENT).asString();

    if (!gameClientId.empty() && m_gameClients.find(gameClientId) == m_gameClients.end())
    {
      GameClientPtr gameClient;
      bool enabled = false;

      ADDON::AddonPtr addon;
      ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();
      if (addonManager.GetAddon(gameClientId, addon, ADDON::AddonType::GAMEDLL,
                                ADDON::OnlyEnabled::CHOICE_YES))
      {
        gameClient = std::static_pointer_cast<CGameClient>(addon);
        enabled = true;
      }
      else if (addonManager.GetAddon(gameClientId, addon, ADDON::AddonType::GAMEDLL,
                                     ADDON::OnlyEnabled::CHOICE_NO))
      {
        gameClient = std::static_pointer_cast<CGameClient>(addon);
        enabled = false;
      }

      if (gameClient)
        CLog::Log(LOGDEBUG, "Game client {} ({}) found for savestate", gameClientId,
                  enabled ? "enabled" : "disabled");
      else
        CLog::Log(LOGDEBUG, "Game client {} not found", gameClientId);

      m_gameClients.emplace(
          std::make_pair(std::move(gameClientId), std::make_pair(std::move(gameClient), enabled)));
    }
  }

  // There's a race condition where the item's focus sends the update message
  // before the window is fully initialized, so explicitly set the info now.
  if (!m_vecList->IsEmpty())
  {
    CFileItemPtr firstItem = m_vecList->Get(0);

    // Get savestate properties
    m_currentGameClientId = firstItem->GetProperty(SAVESTATE_GAME_CLIENT).asString();
    m_currentCaption = firstItem->GetProperty(SAVESTATE_CAPTION).asString();

    // Get game client properties
    std::string emulatorName;
    std::string emulatorIcon;
    const auto it = m_gameClients.find(m_currentGameClientId);
    if (it != m_gameClients.end())
    {
      const GameClientPtr& gameClient = it->second.first;
      if (gameClient)
      {
        emulatorName = gameClient->GetEmulatorName();
        emulatorIcon = gameClient->Icon();
      }
    }

    // Set GUI properties
    if (!emulatorName.empty())
    {
      CGUIMessage message(GUI_MSG_LABEL_SET, GetID(), CONTROL_SAVES_EMULATOR_NAME);
      message.SetLabel(std::move(emulatorName));
      OnMessage(message);
    }
    if (!emulatorIcon.empty())
    {
      CGUIMessage message(GUI_MSG_SET_FILENAME, GetID(), CONTROL_SAVES_EMULATOR_ICON);
      message.SetLabel(std::move(emulatorIcon));
      OnMessage(message);
    }
    if (!m_currentCaption.empty())
    {
      CGUIMessage message(GUI_MSG_LABEL_SET, GetID(), CONTROL_SAVES_DESCRIPTION);
      message.SetLabel(m_currentCaption);
      OnMessage(message);
    }
  }

  // Initialize game clients
  for (auto& [gameClientId, gameClientPair] : m_gameClients)
  {
    GameClientPtr& gameClient = gameClientPair.first;
    bool& enabled = gameClientPair.second;

    if (!gameClient || !enabled)
      continue;

    if (!gameClient->Initialize())
    {
      enabled = false;
      continue;
    }

    /*
    auto m_streamManager = std::make_unique<RETRO_ENGINE::CRetroEngineStreamManager>();

    // Initialize input
    m_input = std::make_unique<CRetroPlayerInput>(CServiceBroker::GetPeripherals(),
      *m_processInfo, m_gameClient);
    m_input->StartAgentManager();

    if (!bStandalone)
    {
      std::string redactedPath = CURL::GetRedacted(fileCopy.GetPath());
      CLog::Log(LOGINFO, "RetroPlayer[PLAYER]: Opening: {}", redactedPath);
      bSuccess = m_gameClient->OpenFile(fileCopy, *m_streamManager, m_input.get());
    }
    */
  }
}

void CDialogGameSaves::OnDeinitWindow(int nextWindowID)
{
  m_viewControl->Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);

  // Get selected item
  for (int i = 0; i < m_vecList->Size(); ++i)
  {
    CFileItemPtr item = m_vecList->Get(i);
    if (item->IsSelected())
    {
      m_selectedItem = item;
      break;
    }
  }

  m_vecList->Clear();

  // Deinitialize game clients
  for (const auto& [gameClientId, gameClientPair] : m_gameClients)
  {
    const GameClientPtr& gameClient = gameClientPair.first;
    bool enabled = gameClientPair.second;
    if (gameClient)
    {
      if (enabled)
      {
        CLog::Log(LOGDEBUG, "Deinitializing game client {}", gameClient->ID());
        gameClient->Unload();
      }
      else
        CLog::Log(LOGDEBUG, "Game client {} was not enabled", gameClient->ID());
    }
  }

  m_gameClients.clear();
  m_currentCaption.clear();
  m_currentGameClientId.clear();
}

void CDialogGameSaves::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl->Reset();
  m_viewControl->SetParentWindow(GetID());
  m_viewControl->AddView(GetControl(CONTROL_SAVES_DETAILED_LIST));
}

void CDialogGameSaves::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl->Reset();
}

void CDialogGameSaves::Reset()
{
  m_bConfirmed = false;
  m_bNewPressed = false;

  m_vecList->Clear();
  m_selectedItem.reset();
}

bool CDialogGameSaves::Open(const std::string& gamePath)
{
  CFileItemList items;

  RETRO::CSavestateDatabase db;
  if (!db.GetSavestatesNav(items, gamePath))
    return false;

  if (items.IsEmpty())
    return false;

  items.Sort(SortByDate, SortOrderDescending);

  SetItems(items);
  m_gamePath = gamePath;

  CGUIDialog::Open();

  return true;
}

std::string CDialogGameSaves::GetSelectedItemPath()
{
  if (m_selectedItem)
    return m_selectedItem->GetPath();

  return "";
}

void CDialogGameSaves::SetItems(const CFileItemList& itemList)
{
  m_vecList->Clear();

  // Need to make internal copy of list to be sure dialog is owner of it
  m_vecList->Copy(itemList);

  m_viewControl->SetItems(*m_vecList);
}

void CDialogGameSaves::OnSelect(const CFileItem& item)
{
  m_bConfirmed = true;
  Close();
}

void CDialogGameSaves::OnFocus(const CFileItem& item)
{
  const std::string caption = item.GetProperty(SAVESTATE_CAPTION).asString();
  const std::string gameClientId = item.GetProperty(SAVESTATE_GAME_CLIENT).asString();

  HandleCaption(caption);
  HandleGameClient(gameClientId);
}

void CDialogGameSaves::OnFocusLost()
{
  HandleCaption("");
  HandleGameClient("");
}

void CDialogGameSaves::OnContextMenu(CFileItem& item)
{
  CContextButtons buttons;

  buttons.Add(0, 118); // "Rename"
  buttons.Add(1, 117); // "Delete"

  const int index = CGUIDialogContextMenu::Show(buttons);

  if (index == 0)
    OnRename(item);
  else if (index == 1)
    OnDelete(item);

  m_viewControl->SetItems(*m_vecList);
}

void CDialogGameSaves::OnRename(CFileItem& item)
{
  const std::string& savestatePath = item.GetPath();

  // Get savestate properties
  RETRO::CSavestateDatabase db;
  std::unique_ptr<RETRO::ISavestate> savestate = RETRO::CSavestateDatabase::AllocateSavestate();
  db.GetSavestate(savestatePath, *savestate);

  std::string label(savestate->Label());

  // "Enter new filename"
  if (CGUIKeyboardFactory::ShowAndGetInput(label, CVariant{g_localizeStrings.Get(16013)}, true) &&
      label != savestate->Label())
  {
    std::unique_ptr<RETRO::ISavestate> newSavestate = db.RenameSavestate(savestatePath, label);
    if (newSavestate)
    {
      RETRO::CSavestateDatabase::GetSavestateItem(*newSavestate, savestatePath, item);

      // Refresh thumbnails
      CGUIMessage message(GUI_MSG_REFRESH_LIST, GetID(), CONTROL_SAVES_DETAILED_LIST);
      OnMessage(message);
    }
    else
    {
      // "Error"
      // "An unknown error has occurred."
      CGUIDialogOK::ShowAndGetInput(257, 24071);
    }
  }
}

void CDialogGameSaves::OnDelete(CFileItem& item)
{
  // "Confirm delete"
  // "Would you like to delete the selected file(s)?[CR]Warning - this action can't be undone!"
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, CVariant{125}))
  {
    const std::string& savestatePath = item.GetPath();

    RETRO::CSavestateDatabase db;
    if (db.DeleteSavestate(savestatePath))
    {
      m_vecList->Remove(&item);

      // Refresh thumbnails
      CGUIMessage message(GUI_MSG_REFRESH_LIST, GetID(), CONTROL_SAVES_DETAILED_LIST);
      OnMessage(message);
    }
    else
    {
      // "Error"
      // "An unknown error has occurred."
      CGUIDialogOK::ShowAndGetInput(257, 24071);
    }
  }
}

void CDialogGameSaves::HandleCaption(const std::string& caption)
{
  if (caption != m_currentCaption)
  {
    m_currentCaption = caption;

    // Update the GUI label
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_SAVES_DESCRIPTION);
    msg.SetLabel(m_currentCaption);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());
  }
}

void CDialogGameSaves::HandleGameClient(const std::string& gameClientId)
{
  // We don't want to clear the last viewed game client
  if (gameClientId.empty())
    return;

  if (gameClientId != m_currentGameClientId)
  {
    m_currentGameClientId = gameClientId;

    // Get game client properties
    std::string emulatorName;
    std::string emulatorIcon;
    const auto it = m_gameClients.find(m_currentGameClientId);
    if (it != m_gameClients.end())
    {
      const GameClientPtr& gameClient = it->second.first;
      if (gameClient)
      {
        emulatorName = gameClient->GetEmulatorName();
        emulatorIcon = gameClient->Icon();
      }
    }

    // Update the GUI elements
    if (!emulatorName.empty())
    {
      CGUIMessage message(GUI_MSG_LABEL_SET, GetID(), CONTROL_SAVES_EMULATOR_NAME);
      message.SetLabel(std::move(emulatorName));
      OnMessage(message);
    }
    if (!emulatorIcon.empty())
    {
      CGUIMessage message(GUI_MSG_SET_FILENAME, GetID(), CONTROL_SAVES_EMULATOR_ICON);
      message.SetLabel(std::move(emulatorIcon));
      OnMessage(message);
    }
  }
}
