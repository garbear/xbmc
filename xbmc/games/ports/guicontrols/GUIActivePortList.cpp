/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIActivePortList.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/agents/windows/GUIAgentDefines.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/input/PhysicalTopology.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerTree.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "messaging/ApplicationMessenger.h"

using namespace KODI;
using namespace ADDON;
using namespace GAME;

CGUIActivePortList::CGUIActivePortList(CGUIWindow& window)
  : m_guiWindow(window), m_vecItems(std::make_unique<CFileItemList>())
{
}

CGUIActivePortList::~CGUIActivePortList()
{
  Deinitialize();
}

bool CGUIActivePortList::Initialize(GameClientPtr gameClient)
{
  // Validate parameters
  if (!gameClient)
    return false;

  // Initialize state
  m_gameClient = std::move(gameClient);

  // Initialize GUI
  Refresh();

  // Register observers
  m_gameClient->Input().RegisterObserver(this);
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CGUIActivePortList::OnEvent);

  return true;
}

void CGUIActivePortList::Deinitialize()
{
  // Unregister observers
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  if (m_gameClient)
    m_gameClient->Input().UnregisterObserver(this);

  // Deinitialize GUI
  CleanupItems();

  // Reset state
  m_gameClient.reset();
}

void CGUIActivePortList::Refresh()
{
  CleanupItems();

  // Add input disabled icon
  AddInputDisabled();

  // Add controllers of active ports
  if (m_gameClient)
    AddItems(m_gameClient->Input().GetActiveControllerTree().GetPorts());

  // Update the GUI
  CGUIMessage msg(GUI_MSG_LABEL_BIND, m_guiWindow.GetID(), CONTROL_ACTIVE_PORT_LIST, 0, 0,
                  m_vecItems.get());
  m_guiWindow.OnMessage(msg);
}

void CGUIActivePortList::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageGamePortsChanged:
    {
      using namespace MESSAGING;
      CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_ACTIVE_PORT_LIST);
      CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow.GetID());
    }
    break;
    default:
      break;
  }
}

void CGUIActivePortList::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) || // Also called on install
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) || // Not called on uninstall
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
      typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    using namespace MESSAGING;
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_ACTIVE_PORT_LIST);
    msg.SetStringParam(event.id);
    CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow.GetID());
  }
}

void CGUIActivePortList::AddInputDisabled()
{
  CFileItemPtr item = std::make_shared<CFileItem>();
  item->SetArt("icon", "DefaultAddonNone.png");
  m_vecItems->Add(std::move(item));
}

void CGUIActivePortList::AddItems(const PortVec& ports)
{
  for (const CPortNode& port : ports)
  {
    // Add controller
    ControllerPtr controller = port.GetActiveController().GetController();
    AddItem(std::move(controller));

    // Add child ports
    AddItems(port.GetActiveController().GetHub().GetPorts());
  }
}

void CGUIActivePortList::AddItem(ControllerPtr controller)
{
  // Check if a controller is connected that provides input
  if (controller && controller->Topology().ProvidesInput())
  {
    // Add GUI item
    CFileItemPtr item = std::make_shared<CFileItem>(controller->Layout().Label());
    item->SetArt("icon", controller->Layout().ImagePath());
    m_vecItems->Add(std::move(item));

    // Remember controller
    m_controllers.emplace_back(std::move(controller));
  }
}

void CGUIActivePortList::CleanupItems()
{
  m_vecItems->Clear();
  m_controllers.clear();
}
