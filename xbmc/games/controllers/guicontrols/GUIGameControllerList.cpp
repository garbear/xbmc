/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameControllerList.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/input/GameClientJoystick.h"
#include "games/addons/input/GameClientTopology.h"
#include "games/agents/GameAgent.h"
#include "games/agents/GameAgentManager.h"
#include "games/controllers/Controller.h"
#include "games/controllers/guicontrols/GUIGameControllerProvider.h"
#include "guilib/GUIListItem.h"
#include "guilib/GUIMessage.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/Variant.h"

#include <algorithm>
#include <cstdlib>

using namespace KODI;
using namespace GAME;

CGUIGameControllerList::CGUIGameControllerList(int parentID,
                                               int controlID,
                                               float posX,
                                               float posY,
                                               float width,
                                               float height,
                                               ORIENTATION orientation,
                                               uint32_t alignment,
                                               const CScroller& scroller)
  : CGUIListContainer(parentID, controlID, posX, posY, width, height, orientation, scroller, 0),
    m_alignment(alignment)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLERLIST;
}

CGUIGameControllerList::CGUIGameControllerList(const CGUIGameControllerList& other)
  : CGUIListContainer(other), m_alignment(other.m_alignment)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLERLIST;
}

CGUIGameControllerList* CGUIGameControllerList::Clone(void) const
{
  return new CGUIGameControllerList(*this);
}

bool CGUIGameControllerList::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_LABEL_BIND:
    {
      if (message.GetControlId() == GetID() && message.GetPointer() && m_gameClient)
      {
        const CFileItemList& messageItems =
            *static_cast<const CFileItemList*>(message.GetPointer());

        std::map<std::string, std::string> gamePorts; // Port address -> controller ID
        for (int i = 0; i < messageItems.Size(); ++i)
        {
          CFileItemPtr messageItem = messageItems.Get(i);
          if (!messageItem)
            continue;

          const std::string& controllerAddress = messageItem->GetPath();

          std::string portAddress;
          std::string controllerId;
          std::tie(portAddress, controllerId) =
              CGameClientTopology::SplitAddress(controllerAddress);
          if (portAddress.empty() || controllerId.empty())
            continue;

          gamePorts.emplace(std::move(portAddress), std::move(controllerId));
        }

        for (const auto& [portAddress, controllerId] : gamePorts)
        {
          const CGameClientInput::JoystickMap& joystickMap = m_gameClient->Input().GetJoystickMap();

          auto it = joystickMap.find(portAddress);
          if (it == joystickMap.end())
            continue;

          std::shared_ptr<CGameClientJoystick> gameClientJoystick = it->second;
          if (!gameClientJoystick)
            continue;

          PERIPHERALS::PeripheralPtr peripheral = gameClientJoystick->GetSource();
          if (!peripheral)
            continue;

          /*
          class PeripheralInputHandler : public JOYSTICK::IInputHandler
          {
          public:
            PeripheralInputHandler(const std::string& controllerAddress, const std::string& controllerId)
              : m_controllerId(controllerId)
            {
            }

            // Implementation of IInputHandler
            std::string ControllerID() const override { return m_controllerId; }
            bool HasFeature(const std::string& feature) const override { return true; }
            bool AcceptsInput(const std::string& feature) const override { return true; }
            bool OnButtonPress(const std::string& feature, bool bPressed) override
            {
              OnDigitalInput();
              return false;
            }
            void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override {}
            bool OnButtonMotion(const std::string& feature,
              float magnitude,
              unsigned int motionTimeMs) override
            {
              OnAnalogInput(magnitude);
              return false;
            }
            bool OnAnalogStickMotion(const std::string& feature,
              float x,
              float y,
              unsigned int motionTimeMs) override
            {
              OnAnalogInput(std::max(std::abs(x), std::abs(y)));
              return false;
            }
            bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override { return false; }
            bool OnWheelMotion(const std::string& feature,
              float position,
              unsigned int motionTimeMs) override
            {
              OnAnalogInput(std::abs(position));
              return false;
            }
            bool OnThrottleMotion(const std::string& feature,
              float position,
              unsigned int motionTimeMs) override
            {
              OnAnalogInput(std::abs(position));
              return false;
            }
            void OnInputFrame() override
            {
              const float analogInput = m_nextAnalogInput;

              // Send input to GUI



              // Update state
              m_lastAnalogInput = m_nextAnalogInput;
              m_nextAnalogInput = 0.0f;
            }

          private:
            void OnDigitalInput()
            {
              m_nextAnalogInput = 1.0f;
            }
            void OnAnalogInput(float magnitude)
            {
              m_nextAnalogInput = std::max(m_nextAnalogInput, magnitude);
            }

            // Input properties
            std::string m_controllerId;

            float m_lastAnalogInput{0.0f};
            float m_nextAnalogInput{0.0f};
          };

          peripheral->RegisterInputHandler(new PeripheralInputHandler(portAddress, controllerId), true);
          */
        }
      }
      break;
    }
    default:
      break;
  }

  return CGUIListContainer::OnMessage(message);
}

void CGUIGameControllerList::UpdateInfo(const CGUIListItem* item)
{
  CGUIListContainer::UpdateInfo(item);

  if (item == nullptr)
    return;

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
    m_listProvider = std::make_unique<CGUIGameControllerProvider>(m_portCount, m_portIndex,
                                                                  m_alignment, GetParentID());
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

void CGUIGameControllerList::SetGameClient(GAME::GameClientPtr gameClient)
{
  m_gameClient = std::move(gameClient);
}

void CGUIGameControllerList::ClearGameClient()
{
  m_gameClient.reset();
}

void CGUIGameControllerList::UpdatePortIndex(int itemNumber,
                                             const std::vector<std::string>& inputPorts)
{
  if (itemNumber <= 0)
    return;

  const int agentIndex = itemNumber - 1; // Item numbers start from 1

  CGameAgentManager& agentManager = CServiceBroker::GetGameServices().GameAgentManager();

  GameAgentVec agents = agentManager.GetAgents();
  if (agentIndex < static_cast<int>(agents.size()))
  {
    const GameAgentPtr& agent = agents.at(agentIndex);
    UpdatePortIndex(agent->GetPeripheral(), inputPorts);
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
