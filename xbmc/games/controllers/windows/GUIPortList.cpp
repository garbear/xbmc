/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPortList.h"

#include "GUIPortDefines.h"
#include "GUIPortWindow.h"
#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/input/GameClientTopology.h" //! @todo Needed?
#include "games/controllers/Controller.h"
#include "games/controllers/guicontrols/GUIPortButton.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerTree.h"
#include "games/controllers/types/PortNode.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace ADDON;
using namespace GAME;

CGUIPortList::CGUIPortList(CGUIWindow* window, GameClientPtr gameClient)
  : m_guiWindow(window), m_gameClient(std::move(gameClient))
{
}

bool CGUIPortList::Initialize()
{
  m_portList = dynamic_cast<CGUIControlGroupList*>(m_guiWindow->GetControl(CONTROL_PORT_LIST));
  m_portButtonTemplate =
      dynamic_cast<CGUIButtonControl*>(m_guiWindow->GetControl(CONTROL_PORT_BUTTON_TEMPLATE));

  if (m_portButtonTemplate)
    m_portButtonTemplate->SetVisible(false);

  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CGUIPortList::OnEvent);

  if (m_portList != nullptr && m_portButtonTemplate != nullptr)
  {
    //! Load from XML
    LoadPorts();

    Refresh();

    return true;
  }

  return false;
}

void CGUIPortList::Deinitialize()
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);

  CleanupButtons();

  m_portList = nullptr;
  m_portButtonTemplate = nullptr;
}

void CGUIPortList::Refresh()
{
  CleanupButtons();

  if (m_portList != nullptr)
  {
    unsigned int buttonId = 0;

    // Iterate the root of the controller tree
    for (const CPortNode& port : m_controllerTree.Ports())
    {
      const std::string& portId = port.PortID();
      if (portId.empty())
      {
        CLog::Log(LOGERROR, "Port for game client {} doesn't have an ID", m_gameClient->ID());
        continue;
      }

      //! @todo Handle port labels better
      // "Port {0:s}"
      const std::string buttonString = g_localizeStrings.Get(35112);
      const std::string buttonLabel = StringUtils::Format(buttonString, portId);
      if (!AddButtons(port, buttonId, buttonLabel))
        break;
    }
  }

  // Try to restore focus to the previously focused port
  if (!m_focusedPort.empty() && m_addressToButton.find(m_focusedPort) != m_addressToButton.end())
  {
    const unsigned int buttonIndex = m_addressToButton[m_focusedPort];
    const int controlId = static_cast<int>(buttonIndex + CONTROL_PORT_BUTTONS_START);

    CGUIMessage msg(GUI_MSG_SETFOCUS, m_guiWindow->GetID(), controlId);
    m_guiWindow->OnMessage(msg);
  }
}

void CGUIPortList::OnFocus(unsigned int buttonIndex)
{
  m_focusedPort = m_buttonToAddress[buttonIndex];
}

void CGUIPortList::OnSelect(unsigned int buttonIndex)
{
  auto it = m_buttonToAddress.find(buttonIndex);
  if (it == m_buttonToAddress.end())
    return;

  const std::string& portAddress = it->second;

  //! @todo
  CPortNode& port = const_cast<CPortNode&>(m_controllerTree.GetPort(portAddress));

  ControllerVector controllers;
  for (const CControllerNode& controllerNode : port.CompatibleControllers())
    controllers.emplace_back(controllerNode.Controller());
  if (controllers.empty())
    return;

  // Get current controller to give initial focus
  ControllerPtr controller = port.ActiveController().Controller();

  auto callback = [this, &port](ControllerPtr controller) {
    OnControllerSelected(port, std::move(controller));
  };
  m_controllerSelectDialog.Initialize(std::move(controllers), std::move(controller), callback);
}

void CGUIPortList::ResetPorts()
{
  // Reload from the add-on
  m_controllerTree = m_gameClient->Input().GetControllerTree();

  //! @todo Reload the GUI

  //Save();
}

void CGUIPortList::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) || // Also called on install
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) || // Not called on uninstall
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
      typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    using namespace MESSAGING;
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow->GetID(), CONTROL_PORT_LIST);
    msg.SetStringParam(event.id);
    CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow->GetID());
  }
}

void CGUIPortList::LoadPorts()
{
  //! @todo Load from XML instead of game client
  m_controllerTree = m_gameClient->Input().GetControllerTree();
}

bool CGUIPortList::AddButtons(const CPortNode& port,
                              unsigned int& buttonId,
                              const std::string& buttonLabel)
{
  if (buttonId + 1 >= MAX_PORT_COUNT)
    return false;

  const CControllerNode& controllerNode = port.ActiveController();
  ControllerPtr controller = controllerNode.Controller();

  // Record the port address so that we can decode button indexes later
  m_buttonToAddress[buttonId] = port.Address();
  m_addressToButton[port.Address()] = buttonId;

  CGUIButtonControl* pButton =
      new CGUIPortButton(*m_portButtonTemplate, buttonLabel, std::move(controller), buttonId++);
  m_portList->AddControl(pButton);

  // Handle buttons for child ports
  const PortVec& ports = controllerNode.Hub().Ports();
  for (const CPortNode& childPort : ports)
  {
    //! @todo Handle port labels better
    // "- Multitap {0:s} Port {1:s}"
    const std::string childButtonString = g_localizeStrings.Get(35113);
    const std::string childButtonLabel =
        StringUtils::Format(childButtonString, port.PortID(), childPort.PortID());
    if (!AddButtons(childPort, buttonId, childButtonLabel))
      return false;
  }

  return true;
}

void CGUIPortList::CleanupButtons()
{
  if (m_portList)
    m_portList->ClearAll();
  m_buttonToAddress.clear();
}

void CGUIPortList::OnControllerSelected(CPortNode& port, ControllerPtr controller)
{
  // Check if user chose "disconnected"
  if (!controller)
  {
    DeactivateController(port);

    // Send a GUI message to reload the port list
    using namespace MESSAGING;
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow->GetID(), CONTROL_PORT_LIST);
    CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow->GetID());

    //Save();

    return;
  }
  else
  {
    const ControllerNodeVec& compatibleControllers = port.CompatibleControllers();

    // Search for user's chosen controller
    for (unsigned int controllerIndex = 0; controllerIndex < compatibleControllers.size();
         ++controllerIndex)
    {
      if (compatibleControllers.at(controllerIndex).Controller()->ID() != controller->ID())
        continue;

      ActivateController(port, controllerIndex);

      // Send a GUI message to reload the port list
      using namespace MESSAGING;
      CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow->GetID(), CONTROL_PORT_LIST);
      msg.SetStringParam(controller->ID());
      CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow->GetID());

      //Save();

      break;
    }
  }
}

void CGUIPortList::ActivateController(CPortNode& port, unsigned int controllerIndex)
{
  // Update the controller tree
  port.SetConnected(true);
  port.SetActiveController(controllerIndex);

  // Update the game client
  ControllerPtr controller = port.CompatibleControllers().at(controllerIndex).Controller();
  if (!m_gameClient->Input().ConnectController(port.Address(), controller))
  {
    //! @todo Show error message on failure
  }

  // Activate child ports
  for (auto& childPort : port.ActiveController().Hub().Ports())
  {
    if (!childPort.CompatibleControllers().empty())
      ActivateController(childPort, 0);
  }
}

void CGUIPortList::DeactivateController(CPortNode& port)
{
  // Check if the controller is already disconnected
  if (!port.Connected())
    return;

  // Disconnect child ports
  for (auto& childPort : port.ActiveController().Hub().Ports())
    DeactivateController(childPort);

  // Update the controller tree
  port.SetConnected(false);

  // Update the game client
  if (!m_gameClient->Input().DisconnectController(port.Address()))
  {
    //! @todo Show error message on failure
  }
}
