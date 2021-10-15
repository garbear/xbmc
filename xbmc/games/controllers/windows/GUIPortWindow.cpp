/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPortWindow.h"

#include "GUIPortDefines.h"
#include "GUIPortList.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "games/addons/GameClient.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIDialog.h" //! @todo Remove me
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace GAME;

CGUIPortWindow::CGUIPortWindow() : CGUIDialog(WINDOW_DIALOG_GAME_PORTS, PORT_DIALOG_XML)
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CGUIPortWindow::~CGUIPortWindow()
{
}

bool CGUIPortWindow::OnMessage(CGUIMessage& message)
{
  // Set to true to block the call to the super class
  bool bHandled = false;

  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int controlId = message.GetSenderId();

      if (controlId == CONTROL_CLOSE_BUTTON)
      {
        Close();
        bHandled = true;
      }
      else if (controlId == CONTROL_RESET_BUTTON)
      {
        ResetPorts();
        bHandled = true;
      }
      else if (CONTROL_PORT_BUTTONS_START <= controlId && controlId < CONTROL_PORT_BUTTONS_END)
      {
        OnPortSelected(controlId - CONTROL_PORT_BUTTONS_START);
        bHandled = true;
      }
      break;
    }
    case GUI_MSG_FOCUSED:
    {
      int controlId = message.GetControlId();

      if (CONTROL_PORT_BUTTONS_START <= controlId && controlId < CONTROL_PORT_BUTTONS_END)
      {
        OnPortFocused(controlId - CONTROL_PORT_BUTTONS_START);
      }
      break;
    }
    case GUI_MSG_REFRESH_LIST:
    {
      UpdatePorts();
      bHandled = true;
      break;
    }
    default:
      break;
  }

  if (!bHandled)
    bHandled = CGUIDialog::OnMessage(message);

  return bHandled;
}

void CGUIPortWindow::OnInitWindow()
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
                                                 ADDON::ADDON_GAMEDLL, ADDON::OnlyEnabled::YES))
        gameClient = std::static_pointer_cast<CGameClient>(addon);
    }
  }
  m_gameClient = std::move(gameClient);

  if (!m_portList)
  {
    m_portList = std::make_unique<CGUIPortList>(this, m_gameClient);
    if (!m_portList->Initialize())
      m_portList.reset();
  }

  UpdatePorts();

  // Set the heading
  SET_CONTROL_LABEL(CONTROL_PORT_DIALOG_LABEL,
                    // "Port Setup"
                    StringUtils::Format("$LOCALIZE[35111] - {}", m_gameClient->Name()));

  // Focus the first port in the list
  CGUIMessage msgFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_PORT_BUTTONS_START);
  OnMessage(msgFocus);
}

void CGUIPortWindow::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  if (m_portList)
  {
    m_portList->Deinitialize();
    m_portList.reset();
  }

  m_gameClient.reset();
}

void CGUIPortWindow::OnPortFocused(unsigned int buttonIndex)
{
  if (m_portList)
    m_portList->OnFocus(buttonIndex);
}

void CGUIPortWindow::OnPortSelected(unsigned int buttonIndex)
{
  if (m_portList)
    m_portList->OnSelect(buttonIndex);
}

void CGUIPortWindow::UpdatePorts()
{
  if (m_portList)
    m_portList->Refresh();
}

void CGUIPortWindow::ResetPorts()
{
  if (m_portList)
    m_portList->ResetPorts();
}
