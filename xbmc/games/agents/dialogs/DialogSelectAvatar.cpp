/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogSelectAvatar.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"
#include "view/GUIViewControl.h"

#include <utility>
#include <vector>

using namespace KODI;
using namespace GAME;

namespace
{
static const std::vector<std::pair<std::string, std::string>> Avatars = {
    {"Mr. Man", "resource://resource.avatar.opengameart/avatars/Mr._Man/01x/001.png"},
    {"Blue Enemy", "resource://resource.avatar.opengameart/avatars/Blue_Enemy/01x/001.png"},
    {"Green Hero", "resource://resource.avatar.opengameart/avatars/Green_Hero/01x/001.png"},
    {"Gum Bot", "resource://resource.avatar.opengameart/avatars/Gum_Bot/01x/001.png"},
    {"Neko", "resource://resource.avatar.opengameart/avatars/Neko/01x/001.png"},
    {"Red Enemy", "resource://resource.avatar.opengameart/avatars/Red_Enemy/01x/001.png"},
    {"PegLeg", "resource://resource.avatar.opengameart/avatars/PegLeg/01x/001.png"},
    {"Samurai", "resource://resource.avatar.opengameart/avatars/Samurai/01x/001.png"},
};
}

CDialogSelectAvatar::CDialogSelectAvatar()
  : CGUIDialog(WINDOW_DIALOG_SELECT_AVATAR, SELECT_AVATAR_DIALOG_XML),
    m_viewControl(std::make_unique<CGUIViewControl>()),
    m_vecItems(std::make_unique<CFileItemList>())
{
}

bool CDialogSelectAvatar::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_SELECT_AVATAR_DETAILED_LIST)
      {
        const int actionId = message.GetParam1();
        switch (actionId)
        {
          case ACTION_SELECT_ITEM:
          case ACTION_MOUSE_LEFT_CLICK:
          case ACTION_TOUCH_TAP:
          case ACTION_PLAYER_PLAY:
          {
            const int selectedItem = m_viewControl->GetSelectedItem();
            if (selectedItem >= 0)
              OnSelect(static_cast<unsigned int>(m_viewControl->GetSelectedItem()));
            break;
          }
          default:
            break;
        }
      }
      return true;
    }
    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

void CDialogSelectAvatar::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  //! @todo
  const unsigned int playerNumber = 1;

  // Create items
  for (const auto& avatar : Avatars)
  {
    CFileItem item(avatar.first);
    item.SetArt("icon", avatar.second);
    m_vecItems->Add(std::move(item));
  }

  // Initialize dialog
  m_viewControl->SetItems(*m_vecItems);
  m_viewControl->SetCurrentView(CONTROL_SELECT_AVATAR_DETAILED_LIST);

  // Set Ready Player label
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_SELECT_AVATAR_READY_PLAYER_LABEL);
  msg.SetLabel(
      StringUtils::Format(g_localizeStrings.Get(35300), playerNumber)); // "Ready Player {0:d}"
  OnMessage(msg);

  // Ensure the list is focused
  CGUIMessage msg2(GUI_MSG_SETFOCUS, GetID(), CONTROL_SELECT_AVATAR_DETAILED_LIST);
  OnMessage(msg2);

  // Reset state
  m_selectedItem = -1;
}

void CDialogSelectAvatar::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  // Set up avatar as Player One
  //! @todo

  if (!IsPlayerOneReady())
  {
    auto appMessenger = CServiceBroker::GetAppMessenger();
    if (appMessenger)
      appMessenger->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                            static_cast<void*>(new CAction(ACTION_STOP)));
  }

  // Reset dialog
  m_viewControl->Clear();
  m_vecItems->Clear();
}

void CDialogSelectAvatar::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  // Set up view control
  m_viewControl->Reset();
  m_viewControl->SetParentWindow(GetID());
  m_viewControl->AddView(GetControl(CONTROL_SELECT_AVATAR_DETAILED_LIST));
}

void CDialogSelectAvatar::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();

  m_viewControl->Reset();
}

bool CDialogSelectAvatar::IsPlayerOneReady()
{
  return m_selectedItem >= 0;
}

void CDialogSelectAvatar::OnSelect(unsigned int itemIndex)
{
  m_selectedItem = itemIndex;
  Close();
}
