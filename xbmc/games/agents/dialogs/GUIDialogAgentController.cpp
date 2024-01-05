/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogAgentController.h"

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
#include "peripherals/Peripherals.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "view/GUIViewControl.h"

#include <string>
#include <utility>
#include <vector>

using namespace KODI;
using namespace GAME;

namespace
{
static const std::vector<std::tuple<std::string, std::string, std::string>> Avatars = {
    /*
    {"Mr. Man", "resource://resource.avatar.opengameart/avatars/Mr._Man/01x/001.png"},
    {"Blue Enemy", "resource://resource.avatar.opengameart/avatars/Blue_Enemy/01x/001.png"},
    {"Green Hero", "resource://resource.avatar.opengameart/avatars/Green_Hero/01x/001.png"},
    {"Gum Bot", "resource://resource.avatar.opengameart/avatars/Gum_Bot/01x/001.png"},
    {"Neko", "resource://resource.avatar.opengameart/avatars/Neko/01x/001.png"},
    {"Red Enemy", "resource://resource.avatar.opengameart/avatars/Red_Enemy/01x/001.png"},
    {"PegLeg", "resource://resource.avatar.opengameart/avatars/PegLeg/01x/001.png"},
    {"Samurai", "resource://resource.avatar.opengameart/avatars/Samurai/01x/001.png"},
*/
    {"Map Player 1", "resource://resource.avatar.opengameart/avatars/Mr._Man/01x/001.png",
     "game.controller.default"},
    {"Map Player 2", "resource://resource.avatar.opengameart/avatars/Blue_Enemy/01x/001.png", ""},
    {"Map Player 3", "resource://resource.avatar.opengameart/avatars/Green_Hero/01x/001.png",
     "game.controller.default"},
    {"Map Player 4", "resource://resource.avatar.opengameart/avatars/Gum_Bot/01x/001.png", ""},
    {"Map Player 5", "resource://resource.avatar.opengameart/avatars/Neko/01x/001.png", ""},
    {"Map Player 6", "resource://resource.avatar.opengameart/avatars/Red_Enemy/01x/001.png", ""},
    {"Map Player 7", "resource://resource.avatar.opengameart/avatars/PegLeg/01x/001.png", ""},
    {"Map Player 8", "resource://resource.avatar.opengameart/avatars/Samurai/01x/001.png", ""},
};
}

CGUIDialogAgentController::CGUIDialogAgentController()
  : CGUIDialog(WINDOW_DIALOG_AGENT_CONTROLLER, DIALOG_AGENT_CONTROLLER_XML),
    m_viewControl(std::make_unique<CGUIViewControl>()),
    m_vecItems(std::make_unique<CFileItemList>())
{
}

bool CGUIDialogAgentController::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      //! @todo
      //m_controllerUid = std::stoi(message.GetStringParam());
      std::string peripheralLocation = message.GetStringParam();
      m_peripheral = CServiceBroker::GetPeripherals().GetPeripheralAtLocation(peripheralLocation);
      break;
    }
    case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_AGENT_CONTROLLER_DETAILED_LIST)
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
            return true;
          }
          default:
            break;
        }
      }
      break;
    }
    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogAgentController::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Create items
  for (const auto& avatar : Avatars)
  {
    CFileItem item(std::get<0>(avatar));
    item.SetArt("icon", std::get<1>(avatar));
    item.SetProperty("game.controllerid", std::get<2>(avatar));
    m_vecItems->Add(std::move(item));
  }

  // Initialize dialog
  m_viewControl->SetItems(*m_vecItems);
  m_viewControl->SetCurrentView(CONTROL_AGENT_CONTROLLER_DETAILED_LIST);

  // Set header
  if (m_peripheral)
  {
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_AGENT_CONTROLLER_HEADER);
    msg.SetLabel(m_peripheral->DeviceName());
    OnMessage(msg);
  }

  // Ensure the list is focused
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_AGENT_CONTROLLER_DETAILED_LIST);
  OnMessage(msg);

  // Reset state
  m_selectedItem = -1;
}

void CGUIDialogAgentController::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  // Reset dialog
  m_viewControl->Clear();
  m_vecItems->Clear();
}

void CGUIDialogAgentController::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  // Set up view control
  m_viewControl->Reset();
  m_viewControl->SetParentWindow(GetID());
  m_viewControl->AddView(GetControl(CONTROL_AGENT_CONTROLLER_DETAILED_LIST));
}

void CGUIDialogAgentController::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();

  m_viewControl->Reset();
}

void CGUIDialogAgentController::OnSelect(unsigned int itemIndex)
{
  m_selectedItem = itemIndex;
  Close();
}
