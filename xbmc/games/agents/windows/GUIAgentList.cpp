/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAgentList.h"

#include "FileItem.h"
#include "GUIAgentDefines.h"
#include "GUIAgentWindow.h"
#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/agents/GameAgent.h"
#include "games/agents/GameAgentManager.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/ControllerManager.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

using namespace KODI;
using namespace ADDON;
using namespace GAME;

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

bool CGUIAgentList::HasControl(int controlId) const
{
  return m_viewControl->HasControl(controlId);
}

int CGUIAgentList::GetCurrentControl() const
{
  return m_viewControl->GetCurrentControl();
}

void CGUIAgentList::FrameMove()
{
  CGUIBaseContainer* thumbs =
      dynamic_cast<CGUIBaseContainer*>(m_guiWindow.GetControl(CONTROL_AGENT_LIST));
  if (thumbs != nullptr)
  {
    const int selectedItem = thumbs->GetSelectedItem();
    if (0 <= selectedItem && selectedItem < m_vecItems->Size())
    {
      const unsigned int focusedItem = static_cast<unsigned int>(selectedItem);
      if (focusedItem != m_currentItem)
        OnItemFocus(focusedItem);
    }
  }
}

void CGUIAgentList::Refresh()
{
  // Send a synchronous message to clear the view control
  m_viewControl->Clear();

  CleanupItems();

  CGameAgentManager& agentManager = CServiceBroker::GetGameServices().GameAgentManager();

  GameAgentVec agents = agentManager.GetAgents();
  for (const GameAgentPtr& agent : agents)
    AddItem(*agent);

  // Add a "No controllers connected" item if no agents are available
  if (m_vecItems->IsEmpty())
  {
    CFileItemPtr item =
        std::make_shared<CFileItem>(g_localizeStrings.Get(35174)); // "No controllers connected"
    m_vecItems->Add(std::move(item));
  }

  // Update items
  m_viewControl->SetItems(*m_vecItems);

  // Try to restore focus to the previously focused agent
  for (unsigned int currentItem = 0; static_cast<int>(currentItem) < m_vecItems->Size();
       ++currentItem)
  {
    CFileItemPtr item = m_vecItems->Get(currentItem);
    if (item && item->GetPath() == m_currentAgent)
    {
      m_viewControl->SetSelectedItem(currentItem);
      break;
    }
  }
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

void CGUIAgentList::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageGameAgentsChanged:
    {
      CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_AGENT_LIST);
      CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow.GetID());
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
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_AGENT_LIST);
    msg.SetStringParam(event.addonId);
    CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow.GetID());
  }
}

void CGUIAgentList::AddItem(const CGameAgent& agent)
{
  // Create the list item from agent properties
  const std::string label = agent.GetPeripheralName();
  const ControllerPtr controller = agent.GetController();
  const std::string path = agent.GetPeripheralLocation();

  CFileItemPtr item = std::make_shared<CFileItem>(label);
  item->SetPath(path);
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

void CGUIAgentList::OnItemFocus(unsigned int itemIndex)
{
  // Remember the focused item
  m_currentItem = itemIndex;

  // Handle the focused agent
  CFileItemPtr item = m_vecItems->Get(m_currentItem);
  if (item)
    OnAgentFocus(item->GetPath());
}

void CGUIAgentList::OnAgentFocus(const std::string& focusedAgent)
{
  if (!focusedAgent.empty())
  {
    // Remember the focused agent
    m_currentAgent = focusedAgent;
  }
}

void CGUIAgentList::OnItemSelect(unsigned int itemIndex)
{
  // Handle the selected agent
  CFileItemPtr item = m_vecItems->Get(itemIndex);
  if (item)
    OnAgentSelect(item->GetPath());
}

void CGUIAgentList::OnAgentSelect(const std::string& selectedAgent)
{
  if (!selectedAgent.empty())
  {
    // Log the selected agent
    CLog::Log(LOGDEBUG, "Selected agent: {}", selectedAgent);
  }
}
