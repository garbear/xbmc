/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAgentProfile.h"

#include "GUIAgentDefines.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "games/addons/GameClient.h"
#include "games/controllers/types/ControllerHub.h" //! @todo Why is this needed to fix compile?
#include "games/ports/guicontrols/GUIActivePortList.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"

using namespace KODI;
using namespace GAME;

CGUIAgentProfile::CGUIAgentProfile()
  : CGUIDialog(WINDOW_DIALOG_AGENT_PROFILE, AGENT_DIALOG_XML),
    m_portList(std::make_unique<CGUIActivePortList>(*this))
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CGUIAgentProfile::~CGUIAgentProfile() = default;

bool CGUIAgentProfile::OnMessage(CGUIMessage& message)
{
  // Set to true to block the call to the super class
  bool bHandled = false;

  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_agentDid = message.GetStringParam(0);
      break;
    }
    case GUI_MSG_CLICKED:
    {
      const int controlId = message.GetSenderId();

      if (controlId == CONTROL_CLOSE_BUTTON)
      {
        CloseDialog();
        bHandled = true;
      }
      if (controlId == CONTROL_RESET_BUTTON)
      {
        ResetProfile();
        bHandled = true;
      }
      break;
    }
    case GUI_MSG_REFRESH_LIST:
    {
      const int controlId = message.GetControlId();
      switch (controlId)
      {
        case CONTROL_ACTIVE_PORT_LIST:
        {
          UpdateActivePortList();
          break;
        }
        default:
          break;
      }
      break;
    }
    default:
      break;
  }

  if (!bHandled)
    bHandled = CGUIDialog::OnMessage(message);

  return bHandled;
}

void CGUIAgentProfile::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  //! @todo
}

void CGUIAgentProfile::OnWindowUnload()
{
  //! @todo

  CGUIDialog::OnWindowUnload();
}

void CGUIAgentProfile::OnInitWindow()
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
                                                 ADDON::ADDON_GAMEDLL,
                                                 ADDON::OnlyEnabled::CHOICE_YES))
        gameClient = std::static_pointer_cast<CGameClient>(addon);
    }
  }
  m_gameClient = std::move(gameClient);

  m_portList->Initialize(m_gameClient);

  UpdateActivePortList();
}

void CGUIAgentProfile::OnDeinitWindow(int nextWindowID)
{
  m_portList->Deinitialize();

  m_gameClient.reset();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIAgentProfile::CloseDialog()
{
  Close();
}

void CGUIAgentProfile::ResetProfile()
{
  //! @todo
}

void CGUIAgentProfile::UpdateActivePortList()
{
  m_portList->Refresh();
}
