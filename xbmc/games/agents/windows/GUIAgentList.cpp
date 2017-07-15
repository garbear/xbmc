/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAgentList.h"

#include "FileItem.h"
#include "GUIAgentDefines.h"
#include "GUIAgentProfile.h"
#include "GUIAgentWindow.h"
#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/agents/GameAgentManager.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/ControllerManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "peripherals/PeripheralTypes.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

using namespace KODI;
using namespace ADDON;
using namespace GAME;

//! @todo
#define PROPERTY_AGENT_DID "Agent.DID"

CGUIAgentList::CGUIAgentList(CGUIWindow& window)
  : m_guiWindow(window),
    m_viewControl(std::make_unique<CGUIViewControl>()),
    m_vecItems(std::make_unique<CFileItemList>())
{
}

CGUIAgentList::~CGUIAgentList()
{
  Deinitialize();
}

void CGUIAgentList::OnWindowLoaded()
{
  m_viewControl->Reset();
  m_viewControl->SetParentWindow(m_guiWindow.GetID());
  m_viewControl->AddView(m_guiWindow.GetControl(CONTROL_AGENT_LIST));
}

void CGUIAgentList::OnWindowUnload()
{
  m_viewControl->Reset();
}

bool CGUIAgentList::Initialize(GameClientPtr gameClient)
{
  // Validate parameters
  if (!gameClient)
    return false;

  // Initialize state
  m_gameClient = std::move(gameClient);
  m_viewControl->SetCurrentView(DEFAULT_VIEW_LIST);

  // Initialize GUI
  Refresh();

  // Register observers
  if (m_gameClient)
    m_gameClient->Input().RegisterObserver(this);
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CGUIAgentList::OnEvent);
  if (CServiceBroker::IsServiceManagerUp())
    CServiceBroker::GetGameServices().GameAgentManager().RegisterObserver(this);

  return true;
}

void CGUIAgentList::Deinitialize()
{
  // Unregister observers in reverse order
  if (CServiceBroker::IsServiceManagerUp())
    CServiceBroker::GetGameServices().GameAgentManager().UnregisterObserver(this);
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  if (m_gameClient)
    m_gameClient->Input().UnregisterObserver(this);

  // Deinitialize GUI
  CleanupItems();

  // Reset state
  m_gameClient.reset();
}

bool CGUIAgentList::HasControl(int controlId)
{
  return m_viewControl->HasControl(controlId);
}

int CGUIAgentList::GetCurrentControl()
{
  return m_viewControl->GetCurrentControl();
}

void CGUIAgentList::Refresh()
{
  // Send a synchronous message to clear the view control
  m_viewControl->Clear();

  CleanupItems();

  CGameAgentManager& agentManager = CServiceBroker::GetGameServices().GameAgentManager();

  PERIPHERALS::PeripheralVector agents = agentManager.GetAgents();
  for (const PERIPHERALS::PeripheralPtr& agent : agents)
    AddItem(*agent);

  m_viewControl->SetItems(*m_vecItems);

  //! @todo Try to restore focus to the previously focused agent instead of item
  // index
  m_viewControl->SetSelectedItem(m_currentItem);
}

void CGUIAgentList::SetFocused()
{
  m_viewControl->SetFocused();
}

void CGUIAgentList::OnSelect()
{
  const int itemIndex = m_viewControl->GetSelectedItem();
  if (itemIndex >= 0)
    OnItemSelect(static_cast<unsigned int>(itemIndex));
}

void CGUIAgentList::ResetAgents()
{
  //! @todo
}

void CGUIAgentList::AddAgent()
{
  m_identityProviderSelectDialog.Initialize();
}

void CGUIAgentList::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageGameAgentsChanged:
    case ObservableMessageGamePortsChanged:
    {
      using namespace MESSAGING;
      CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_AGENT_LIST);
      CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow.GetID());
    }
    break;
    default:
      break;
  }
}

void CGUIAgentList::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) || // Also called on install
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) || // Not called on uninstall
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
      typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    using namespace MESSAGING;
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_AGENT_LIST);
    msg.SetStringParam(event.id);
    CApplicationMessenger::GetInstance().SendGUIMessage(msg, m_guiWindow.GetID());
  }
}

void CGUIAgentList::AddItem(const PERIPHERALS::CPeripheral& agent)
{
  // Create the list item from peripheral properties
  std::string label = agent.DeviceName();
  ControllerPtr controller = agent.ControllerProfile();

  // Handle empty label
  if (label.empty())
    label = controller->Layout().Label();

  CFileItemPtr item = std::make_shared<CFileItem>(label);
  if (controller)
  {
    item->SetProperty("Addon.ID", controller->ID());
    item->SetArt("icon", controller->Layout().ImagePath());
  }
  m_vecItems->Add(std::move(item));
}

void CGUIAgentList::CleanupItems()
{
  m_vecItems->Clear();
}

void CGUIAgentList::OnItemSelect(unsigned int itemIndex)
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui == nullptr)
    return;

  CGUIWindowManager& windowManager = gui->GetWindowManager();

  auto pAgentDialog = windowManager.GetWindow<CGUIAgentProfile>(WINDOW_DIALOG_AGENT_PROFILE);
  if (pAgentDialog == nullptr)
    return;

  const std::string agentDid = std::to_string(itemIndex + 1); //! @todo
  pAgentDialog->SetProperty(PROPERTY_AGENT_DID, StringUtils::Format("Player {}", agentDid));

  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_AGENT_PROFILE,
                                                              agentDid);
}
