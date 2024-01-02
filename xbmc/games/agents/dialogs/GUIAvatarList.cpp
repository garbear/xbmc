/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAvatarList.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIAvatarDefines.h"
#include "GUIAvatarDialog.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/agents/avatars/Avatar.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerTree.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

using namespace KODI;
using namespace GAME;

CGUIAvatarList::CGUIAvatarList(CGUIWindow& window)
  : m_guiWindow(window),
    m_viewControl(std::make_unique<CGUIViewControl>()),
    m_vecItems(std::make_unique<CFileItemList>())
{
}

CGUIAvatarList::~CGUIAvatarList()
{
  Deinitialize();
}

void CGUIAvatarList::OnWindowLoaded()
{
  m_viewControl->Reset();
  m_viewControl->SetParentWindow(m_guiWindow.GetID());
  m_viewControl->AddView(m_guiWindow.GetControl(CONTROL_AVATAR_LIST));
}

void CGUIAvatarList::OnWindowUnload()
{
  m_viewControl->Reset();
}

bool CGUIAvatarList::Initialize(GameClientPtr gameClient)
{
  // Validate parameters
  if (!gameClient)
    return false;

  // Initialize state
  m_gameClient = std::move(gameClient);
  m_viewControl->SetCurrentView(DEFAULT_VIEW_LIST);

  // Initialize GUI
  Refresh();

  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CGUIAvatarList::OnEvent);

  return true;
}

void CGUIAvatarList::Deinitialize()
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);

  // Deinitialize GUI
  CleanupItems();

  // Reset state
  m_gameClient.reset();
}

bool CGUIAvatarList::HasControl(int controlId)
{
  return m_viewControl->HasControl(controlId);
}

int CGUIAvatarList::GetCurrentControl()
{
  return m_viewControl->GetCurrentControl();
}

void CGUIAvatarList::Refresh()
{
  // Send a synchronous message to clear the view control
  m_viewControl->Clear();

  CleanupItems();

  if (m_gameClient)
  {
    CControllerTree controllerTree = m_gameClient->Input().GetActiveControllerTree();

    unsigned int itemIndex = 0;
    for (const CPortNode& avatar : controllerTree.GetPorts())
      AddItems(avatar, itemIndex, GetLabel(avatar));

    m_viewControl->SetItems(*m_vecItems);

    // Try to restore focus to the previously focused avatar
    if (!m_focusedAvatar.empty() && m_addressToItem.find(m_focusedAvatar) != m_addressToItem.end())
    {
      const unsigned int itemIndex = m_addressToItem[m_focusedAvatar];
      m_viewControl->SetSelectedItem(itemIndex);
      OnItemFocus(itemIndex);
    }
  }
}

void CGUIAvatarList::FrameMove()
{
  const int itemIndex = m_viewControl->GetSelectedItem();
  if (itemIndex != m_currentItem)
  {
    m_currentItem = itemIndex;
    if (itemIndex >= 0)
      OnItemFocus(static_cast<unsigned int>(itemIndex));
  }
}

void CGUIAvatarList::SetFocused()
{
  m_viewControl->SetFocused();
}

bool CGUIAvatarList::OnSelect()
{
  const int itemIndex = m_viewControl->GetSelectedItem();
  if (itemIndex >= 0)
  {
    OnItemSelect(static_cast<unsigned int>(itemIndex));
    return true;
  }

  return false;
}

void CGUIAvatarList::ResetAvatars()
{
  if (m_gameClient)
  {
    // Update the game client
    m_gameClient->Input().ResetPorts();
    m_gameClient->Input().SavePorts();

    // Refresh the GUI
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_AVATAR_LIST);
    CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow.GetID());
  }
}

void CGUIAvatarList::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) || // Also called on install
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) || // Not called on uninstall
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
      typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_AVATAR_LIST);
    msg.SetStringParam(event.addonId);
    CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow.GetID());
  }
}

bool CGUIAvatarList::AddItems(const CPortNode& avatar,
                              unsigned int& itemId,
                              const std::string& itemLabel)
{
  // Validate parameters
  if (itemLabel.empty())
    return false;

  // Record the avatar address so that we can decode item indexes later
  m_itemToAddress[itemId] = avatar.GetAddress();
  m_addressToItem[avatar.GetAddress()] = itemId;

  if (avatar.IsConnected())
  {
    const CControllerNode& controllerNode = avatar.GetActiveController();
    const ControllerPtr& controller = controllerNode.GetController();

    // Create the list item
    CFileItemPtr item = std::make_shared<CFileItem>(itemLabel);
    item->SetLabel2(controller->Layout().Label());
    item->SetPath(avatar.GetAddress());
    item->SetArt("icon", controller->Layout().ImagePath());
    m_vecItems->Add(std::move(item));
    ++itemId;

    // Handle items for child avatars
    const PortVec& ports = controllerNode.GetHub().GetPorts();
    for (const CPortNode& childPort : ports)
    {
      std::ostringstream childItemLabel;
      childItemLabel << " - ";
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
    item->SetPath(avatar.GetAddress());
    item->SetArt("icon", "DefaultAddonNone.png");
    m_vecItems->Add(std::move(item));
    ++itemId;
  }

  return true;
}

void CGUIAvatarList::CleanupItems()
{
  m_vecItems->Clear();
  m_itemToAddress.clear();
  m_addressToItem.clear();
}

void CGUIAvatarList::OnItemFocus(unsigned int itemIndex)
{
  m_focusedAvatar = m_itemToAddress[itemIndex];
}

void CGUIAvatarList::OnItemSelect(unsigned int itemIndex)
{
  if (m_gameClient)
  {
    const auto it = m_itemToAddress.find(itemIndex);
    if (it == m_itemToAddress.end())
      return;

    const std::string& portAddress = it->second;
    if (portAddress.empty())
      return;

    CPortNode avatar = m_gameClient->Input().GetActiveControllerTree().GetPort(portAddress);

    ControllerVector controllers;
    for (const CControllerNode& controllerNode : avatar.GetCompatibleControllers())
      controllers.emplace_back(controllerNode.GetController());

    // Get current controller to give initial focus
    ControllerPtr controller = avatar.GetActiveController().GetController();

    // Check if we should show a "disconnect" option
    const bool showDisconnect = !avatar.IsForceConnected();

    auto callback = [this, avatar = std::move(avatar)](const ControllerPtr& controller)
    { OnControllerSelected(avatar, controller); };

    m_controllerSelectDialog.Initialize(std::move(controllers), std::move(controller),
                                        showDisconnect, callback);
  }
}

void CGUIAvatarList::OnControllerSelected(const CPortNode& avatar, const ControllerPtr& controller)
{
  if (m_gameClient)
  {
    // Translate parameter
    const bool bConnected = static_cast<bool>(controller);

    // Update the game client
    const bool bSuccess =
        bConnected ? m_gameClient->Input().ConnectController(avatar.GetAddress(), controller)
                   : m_gameClient->Input().DisconnectController(avatar.GetAddress());

    if (bSuccess)
    {
      //! @todo
    }
    else
    {
      // "Failed to change controller"
      // "The emulator "%s" had an internal error."
      MESSAGING::HELPERS::ShowOKDialogText(
          CVariant{35114},
          CVariant{StringUtils::Format(g_localizeStrings.Get(35213), m_gameClient->Name())});
    }

    // Send a GUI message to reload the avatar list
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_AVATAR_LIST);
    CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow.GetID());
  }
}

std::string CGUIAvatarList::GetLabel(const CPortNode& avatar)
{
  return "";
}
