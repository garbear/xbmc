/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameControllerList.h"

#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/agents/GameAgentManager.h"
#include "games/controllers/Controller.h"
#include "games/controllers/guicontrols/GUIGameControllerProvider.h"
#include "guilib/GUIListItem.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

CGUIGameControllerList::CGUIGameControllerList(int parentID,
                                               int controlID,
                                               float posX,
                                               float posY,
                                               float width,
                                               float height,
                                               ORIENTATION orientation,
                                               const CScroller& scroller)
  : CGUIListContainer(parentID, controlID, posX, posY, width, height, orientation, scroller, 0)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLERLIST;
}

CGUIGameControllerList::CGUIGameControllerList(const CGUIGameControllerList& other)
  : CGUIListContainer(other)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLERLIST;
}

CGUIGameControllerList* CGUIGameControllerList::Clone(void) const
{
  return new CGUIGameControllerList(*this);
}

void CGUIGameControllerList::UpdateInfo(const CGUIListItem* item)
{
  CGUIListContainer::UpdateInfo(item);

  CGameAgentManager& agentManager = CServiceBroker::GetGameServices().GameAgentManager();

  // Update port count
  const std::vector<std::string> inputPorts = agentManager.GetInputPorts();
  m_portCount = inputPorts.size();

  // Update port index
  UpdatePortIndex(item->GetCurrentItem(), inputPorts);

  bool bUpdateListProvider = false;

  // Update CGUIListContainer
  if (!m_listProvider)
  {
    m_listProvider =
        std::make_unique<CGUIGameControllerProvider>(m_portCount, m_portIndex, GetParentID());
    bUpdateListProvider = true;
  }

  // Update controller provider
  CGUIGameControllerProvider* controllerProvider =
      dynamic_cast<CGUIGameControllerProvider*>(m_listProvider.get());
  if (controllerProvider != nullptr)
  {
    // Update port count
    if (controllerProvider->GetPortCount() != m_portCount)
    {
      controllerProvider->SetPortCount(m_portCount);
      bUpdateListProvider = true;
    }

    // Update player count
    if (controllerProvider->GetPortIndex() != m_portIndex)
    {
      controllerProvider->SetPortIndex(m_portIndex);
      bUpdateListProvider = true;
    }

    // Update current controller
    const std::string newControllerId = item->GetProperty("Addon.ID").asString();
    if (!newControllerId.empty())
    {
      std::string currentControllerId;
      if (controllerProvider->GetControllerProfile())
        currentControllerId = controllerProvider->GetControllerProfile()->ID();

      if (currentControllerId != newControllerId)
      {
        CGameServices& gameServices = CServiceBroker::GetGameServices();
        ControllerPtr controller = gameServices.GetController(newControllerId);
        controllerProvider->SetControllerProfile(std::move(controller));
        bUpdateListProvider = true;
      }
    }
  }

  if (bUpdateListProvider)
    UpdateListProvider(true);
}

void CGUIGameControllerList::UpdatePortIndex(int itemNumber,
                                             const std::vector<std::string>& inputPorts)
{
  if (itemNumber <= 0)
    return;

  const int agentIndex = itemNumber - 1; // Item numbers start from 1

  CGameAgentManager& agentManager = CServiceBroker::GetGameServices().GameAgentManager();

  PERIPHERALS::PeripheralVector agentPeripherals = agentManager.GetAgents();
  if (agentIndex < static_cast<int>(agentPeripherals.size()))
  {
    const PERIPHERALS::PeripheralPtr& agentPeripheral = agentPeripherals.at(agentIndex);
    UpdatePortIndex(agentPeripheral, inputPorts);
  }
}

void CGUIGameControllerList::UpdatePortIndex(const PERIPHERALS::PeripheralPtr& agentPeripheral,
                                             const std::vector<std::string>& inputPorts)
{
  CGameAgentManager& agentManager = CServiceBroker::GetGameServices().GameAgentManager();

  // Upcast peripheral to input provider
  JOYSTICK::IInputProvider* const inputProvider =
      static_cast<JOYSTICK::IInputProvider*>(agentPeripheral.get());

  // See if the input provider has a port address
  const std::string& portAddress = agentManager.GetPortAddress(inputProvider);
  if (portAddress.empty())
    return;

  m_portIndex = -1;

  // Search ports for input provider's address
  for (size_t i = 0; i < inputPorts.size(); ++i)
  {
    if (inputPorts.at(i) == portAddress)
    {
      // Found port address, record index
      m_portIndex = i;
      break;
    }
  }
}
