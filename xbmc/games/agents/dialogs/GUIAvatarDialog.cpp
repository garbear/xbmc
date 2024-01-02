/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAvatarDialog.h"

#include "GUIAvatarDefines.h"
#include "GUIAvatarList.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "games/addons/GameClient.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "input/actions/ActionIDs.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace GAME;

CGUIAvatarDialog::CGUIAvatarDialog()
  : CGUIDialog(WINDOW_DIALOG_GAME_AVATARS, AVATAR_DIALOG_XML),
    m_avatarList(std::make_unique<CGUIAvatarList>(*this))
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CGUIAvatarDialog::~CGUIAvatarDialog() = default;

bool CGUIAvatarDialog::OnMessage(CGUIMessage& message)
{
  // Set to true to block the call to the super class
  bool bHandled = false;

  switch (message.GetMessage())
  {
    case GUI_MSG_SETFOCUS:
    {
      const int controlId = message.GetControlId();
      if (m_avatarList->HasControl(controlId) && m_avatarList->GetCurrentControl() != controlId)
      {
        FocusAvatarList();
        bHandled = true;
      }
      break;
    }
    case GUI_MSG_CLICKED:
    {
      const int controlId = message.GetSenderId();

      if (controlId == CONTROL_AVATAR_CLOSE_BUTTON)
      {
        CloseDialog();
        bHandled = true;
      }
      /* @todo
      else if (controlId == CONTROL_RESET_BUTTON)
      {
        ResetAvatars();
        bHandled = true;
      }
      */
      else if (m_avatarList->HasControl(controlId))
      {
        const int actionId = message.GetParam1();
        if (actionId == ACTION_SELECT_ITEM || actionId == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClickAction();
          bHandled = true;
        }
      }
      break;
    }
    case GUI_MSG_REFRESH_LIST:
    {
      UpdateAvatarList();
      break;
    }
    default:
      break;
  }

  if (!bHandled)
    bHandled = CGUIDialog::OnMessage(message);

  return bHandled;
}

void CGUIAvatarDialog::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_avatarList->OnWindowLoaded();
}

void CGUIAvatarDialog::OnWindowUnload()
{
  m_avatarList->OnWindowUnload();

  CGUIDialog::OnWindowUnload();
}

void CGUIAvatarDialog::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Get active game add-on
  GameClientPtr gameClient;
  {
    auto gameSettingsHandle = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
    if (gameSettingsHandle)
    {
      ADDON::AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(gameSettingsHandle->GameClientID(), addon,
                                                 ADDON::AddonType::GAMEDLL,
                                                 ADDON::OnlyEnabled::CHOICE_YES))
        gameClient = std::static_pointer_cast<CGameClient>(addon);
    }
  }
  m_gameClient = std::move(gameClient);

  /* @todo
  // Set the heading
  // "Avatar Setup - {game client name}"
  SET_CONTROL_LABEL(CONTROL_AVATAR_DIALOG_LABEL,
                    StringUtils::Format("$LOCALIZE[35111] - {}", m_gameClient->Name()));
  */

  m_avatarList->Initialize(m_gameClient);

  UpdateAvatarList();

  // Focus the avatar list
  CGUIMessage msgFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_AVATAR_LIST);
  OnMessage(msgFocus);
}

void CGUIAvatarDialog::OnDeinitWindow(int nextWindowID)
{
  m_avatarList->Deinitialize();

  m_gameClient.reset();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIAvatarDialog::FrameMove()
{
  CGUIDialog::FrameMove();

  m_avatarList->FrameMove();
}

void CGUIAvatarDialog::UpdateAvatarList()
{
  m_avatarList->Refresh();
}

void CGUIAvatarDialog::FocusAvatarList()
{
  m_avatarList->SetFocused();
}

bool CGUIAvatarDialog::OnClickAction()
{
  return m_avatarList->OnSelect();
}

void CGUIAvatarDialog::ResetAvatars()
{
  m_avatarList->ResetAvatars();
}

void CGUIAvatarDialog::CloseDialog()
{
  Close();
}
