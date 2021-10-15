/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPortList.h"

#include "FileItem.h"
#include "GUIPortDefines.h"
#include "GUIPortWindow.h"
#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/input/GameClientTopology.h" //! @todo Needed?
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerTree.h"
#include "games/controllers/types/PortNode.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

using namespace KODI;
using namespace ADDON;
using namespace GAME;

CGUIPortList::CGUIPortList(CGUIWindow& window)
  : m_guiWindow(window),
    m_viewControl(std::make_unique<CGUIViewControl>()),
    m_vecItems(std::make_unique<CFileItemList>())
{
}

CGUIPortList::~CGUIPortList()
{
  Deinitialize();
}

void CGUIPortList::OnWindowLoaded()
{
  m_viewControl->Reset();
  m_viewControl->SetParentWindow(m_guiWindow.GetID());
  m_viewControl->AddView(m_guiWindow.GetControl(CONTROL_PORT_LIST));
}

void CGUIPortList::OnWindowUnload()
{
  m_viewControl->Reset();
}

bool CGUIPortList::Initialize(GameClientPtr gameClient)
{
  m_gameClient = std::move(gameClient);

  m_viewControl->SetCurrentView(DEFAULT_VIEW_LIST);

  Refresh();

  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CGUIPortList::OnEvent);

  return true;
}

void CGUIPortList::Deinitialize()
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);

  CleanupItems();

  m_gameClient.reset();
}

void CGUIPortList::Refresh()
{
  // Send a message to clear the view control
  m_viewControl->Clear();

  CleanupItems();

  unsigned int itemIndex = 0;
  for (const CPortNode& port : m_gameClient->Input().GetActiveControllerTree().Ports())
    AddItems(port, itemIndex, GetLabel(port));

  m_viewControl->SetItems(*m_vecItems);

  // Try to restore focus to the previously focused port
  if (!m_focusedPort.empty() && m_addressToItem.find(m_focusedPort) != m_addressToItem.end())
  {
    const unsigned int itemIndex = m_addressToItem[m_focusedPort];
    m_viewControl->SetSelectedItem(itemIndex);
    OnItemFocus(itemIndex);
  }
}

bool CGUIPortList::HasControl(int controlId)
{
  return m_viewControl->HasControl(controlId);
}

int CGUIPortList::GetCurrentControl()
{
  return m_viewControl->GetCurrentControl();
}

void CGUIPortList::FrameMove()
{
  const int itemIndex = m_viewControl->GetSelectedItem();
  if (itemIndex != m_currentItem)
  {
    m_currentItem = itemIndex;
    if (itemIndex >= 0)
      OnItemFocus(static_cast<unsigned int>(itemIndex));
  }
}

void CGUIPortList::SetFocused()
{
  m_viewControl->SetFocused();
}

void CGUIPortList::OnSelect()
{
  const int itemIndex = m_viewControl->GetSelectedItem();
  if (itemIndex >= 0)
    OnItemSelect(static_cast<unsigned int>(itemIndex));
}

void CGUIPortList::ResetPorts()
{
  m_gameClient->Input().ResetPorts();

  // Refresh the GUI
  using namespace MESSAGING;
  CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_PORT_LIST);
  CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow.GetID());
}

void CGUIPortList::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) || // Also called on install
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) || // Not called on uninstall
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
      typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    using namespace MESSAGING;
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_PORT_LIST);
    msg.SetStringParam(event.id);
    CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow.GetID());
  }
}

bool CGUIPortList::AddItems(const CPortNode& port,
                            unsigned int& itemId,
                            const std::string& itemLabel)
{
  // Validate parameters
  if (itemLabel.empty())
    return false;

  // Record the port address so that we can decode item indexes later
  m_itemToAddress[itemId] = port.Address();
  m_addressToItem[port.Address()] = itemId;

  if (port.Connected())
  {
    const CControllerNode& controllerNode = port.ActiveController();
    ControllerPtr controller = controllerNode.Controller();

    // Create the list item
    CFileItemPtr item = std::make_shared<CFileItem>(itemLabel);
    item->SetLabel2(controller->Layout().Label());
    item->SetPath(port.Address());
    item->SetArt("icon", controller->Layout().ImagePath());
    m_vecItems->Add(std::move(item));
    ++itemId;

    // Handle items for child ports
    const PortVec& ports = controllerNode.Hub().Ports();
    for (const CPortNode& childPort : ports)
    {
      std::ostringstream childItemLabel;
      childItemLabel << controller->Layout().Label();
      childItemLabel << " - ";
      childItemLabel << GetLabel(childPort);

      if (!AddItems(childPort, itemId, childItemLabel.str()))
        return false;
    }
  }
  else
  {
    // Create the list item
    CFileItemPtr item = std::make_shared<CFileItem>(itemLabel);
    item->SetLabel2(g_localizeStrings.Get(13298)); // "Disconnected"
    item->SetPath(port.Address());
    item->SetArt("icon", "DefaultAddonNone.png");
    m_vecItems->Add(std::move(item));
    ++itemId;
  }

  return true;
}

void CGUIPortList::CleanupItems()
{
  m_vecItems->Clear();
  m_itemToAddress.clear();
  m_addressToItem.clear();
}

void CGUIPortList::OnItemFocus(unsigned int itemIndex)
{
  m_focusedPort = m_itemToAddress[itemIndex];
}

void CGUIPortList::OnItemSelect(unsigned int itemIndex)
{
  const auto it = m_itemToAddress.find(itemIndex);
  if (it == m_itemToAddress.end())
    return;

  const std::string& portAddress = it->second;
  if (portAddress.empty())
    return;

  const CPortNode& port = m_gameClient->Input().GetActiveControllerTree().GetPort(portAddress);

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

void CGUIPortList::OnControllerSelected(const CPortNode& port, ControllerPtr controller)
{
  // Check if user chose "disconnected"
  if (!controller)
  {
    // Update the game client
    if (!m_gameClient->Input().DisconnectController(port.Address()))
    {
      //! @todo Show error message on failure
    }
  }
  else
  {
    // Update the game client
    if (!m_gameClient->Input().ConnectController(port.Address(), controller))
    {
      //! @todo Show error message on failure
    }
  }

  // Send a GUI message to reload the port list
  using namespace MESSAGING;
  CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_PORT_LIST);
  CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow.GetID());
}

std::string CGUIPortList::GetLabel(const CPortNode& port)
{
  const PORT_TYPE portType = port.PortType();
  switch (portType)
  {
    case PORT_TYPE::KEYBOARD:
    {
      // "Keyboard"
      return g_localizeStrings.Get(35150);
    }
    case PORT_TYPE::MOUSE:
    {
      // "Mouse"
      return g_localizeStrings.Get(35171);
    }
    case PORT_TYPE::CONTROLLER:
    {
      const std::string& portId = port.PortID();
      if (portId.empty())
      {
        CLog::Log(LOGERROR, "Controller port with address \"{}\" doesn't have a port ID",
                  port.Address());
      }
      else
      {
        // "Port {0:s}"
        const std::string portString = g_localizeStrings.Get(35112);
        return StringUtils::Format(portString, portId);
      }
      break;
    }
    default:
      break;
  }

  return "";
}
