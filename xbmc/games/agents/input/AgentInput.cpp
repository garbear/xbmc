/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentInput.h"

#include "AgentController.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/input/GameClientJoystick.h"
#include "games/controllers/Controller.h"
#include "input/InputManager.h"
#include "peripherals/EventLockHandle.h"
#include "peripherals/Peripherals.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "utils/log.h"

#include <algorithm>
#include <array>

using namespace KODI;
using namespace GAME;

CAgentInput::CAgentInput(PERIPHERALS::CPeripherals& peripheralManager, CInputManager& inputManager)
  : m_peripheralManager(peripheralManager), m_inputManager(inputManager)
{
  // Register callbacks
  m_peripheralManager.RegisterObserver(this);
  m_inputManager.RegisterKeyboardDriverHandler(this);
  m_inputManager.RegisterMouseDriverHandler(this);
}

CAgentInput::~CAgentInput()
{
  // Unregister callbacks in reverse order
  m_inputManager.UnregisterMouseDriverHandler(this);
  m_inputManager.UnregisterKeyboardDriverHandler(this);
  m_peripheralManager.UnregisterObserver(this);
}

void CAgentInput::Start(GameClientPtr gameClient)
{
  // Initialize state
  m_gameClient = std::move(gameClient);

  // Register callbacks
  if (m_gameClient)
    m_gameClient->Input().RegisterObserver(this);
}

void CAgentInput::Stop()
{
  // Unregister callbacks in reverse order
  if (m_gameClient)
    m_gameClient->Input().UnregisterObserver(this);

  // Close open joysticks
  {
    PERIPHERALS::EventLockHandlePtr inputHandlingLock;

    for (const auto& [inputProvider, joystick] : m_portMap)
    {
      if (!inputHandlingLock)
        inputHandlingLock = m_peripheralManager.RegisterEventLock();

      joystick->UnregisterInput(inputProvider);

      SetChanged(true);
    }

    m_portMap.clear();
  }

  // Notify observers if anything changed
  NotifyObservers(ObservableMessageAgentControllersChanged);

  // Reset state
  m_gameClient.reset();
}

void CAgentInput::Refresh()
{
  if (m_gameClient)
  {
    // Open keyboard
    ProcessKeyboard();

    // Open mouse
    ProcessMouse();

    // Open/close joysticks
    PERIPHERALS::EventLockHandlePtr inputHandlingLock;
    ProcessJoysticks(inputHandlingLock);
  }

  // Notify observers if anything changed
  NotifyObservers(ObservableMessageAgentControllersChanged);
}

void CAgentInput::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageGamePortsChanged:
    case ObservableMessagePeripheralsChanged:
    {
      Refresh();
      break;
    }
    default:
      break;
  }
}

bool CAgentInput::OnKeyPress(const CKey& key)
{
  m_bHasKeyboard = true;
  return false;
}

bool CAgentInput::OnPosition(int x, int y)
{
  m_bHasMouse = true;
  return false;
}

bool CAgentInput::OnButtonPress(MOUSE::BUTTON_ID button)
{
  m_bHasMouse = true;
  return false;
}

std::vector<std::shared_ptr<CAgentController>> CAgentInput::GetControllers() const
{
  std::lock_guard<std::mutex> lock(m_controllerMutex);
  return m_controllers;
}

std::string CAgentInput::GetPortAddress(const std::string& peripheralLocation) const
{
  std::string portAddress;

  std::lock_guard<std::mutex> lock(m_agentMutex);

  auto it = std::find_if(m_controllers.begin(), m_controllers.end(),
                         [&peripheralLocation](const std::shared_ptr<CAgentController>& controller)
                         { return controller->GetPeripheralLocation() == peripheralLocation; });
  if (it != m_controllers.end())
    portAddress = (*it)->GetPortAddress();

  return portAddress;
}

std::vector<std::string> CAgentInput::GetInputPorts() const
{
  std::vector<std::string> inputPorts;

  if (m_gameClient)
  {
    CControllerTree controllerTree = m_gameClient->Input().GetActiveControllerTree();
    controllerTree.GetInputPorts(inputPorts);
  }

  return inputPorts;
}

float CAgentInput::GetPortActivation(const std::string& portAddress) const
{
  float activation = 0.0f;
  JoystickMap joystickMap = m_gameClient->Input().GetJoystickMap();

  auto it = joystickMap.find(portAddress);
  if (it != joystickMap.end())
    activation = it->second->GetActivation();

  return activation;
}

float CAgentInput::GetPeripheralActivation(const std::string& peripheralLocation) const
{
  std::lock_guard<std::mutex> lock(m_controllerMutex);

  for (const std::shared_ptr<CAgentController>& controller : m_controllers)
  {
    if (controller->GetPeripheralLocation() == peripheralLocation)
      return controller->GetActivation();
  }

  return 0.0f;
}

void CAgentInput::ProcessJoysticks(PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  // Get system joysticks.
  //
  // It's important to hold these shared pointers for the function scope
  // because we call into the input handlers in m_portMap.
  //
  // The input handlers are upcasted from their peripheral object, so the
  // peripheral object must persist through this function.
  //
  PERIPHERALS::PeripheralVector joysticks;
  m_peripheralManager.GetPeripheralsWithFeature(joysticks, PERIPHERALS::FEATURE_JOYSTICK);

  // Remove "virtual" Android joysticks
  //
  // The heuristic used to identify these is to check if the device name is all
  // lowercase letters and dashes (and contains at least one dash). The
  // following virtual devices have been observed:
  //
  //   shield-ask-remote
  //   sunxi-ir-uinput
  //   virtual-search
  //
  // Additionally, we specifically allow the following devices:
  //
  //   virtual-remote
  //
  joysticks.erase(
      std::remove_if(joysticks.begin(), joysticks.end(),
                     [](const PERIPHERALS::PeripheralPtr& joystick)
                     {
                       const std::string& joystickName = joystick->DeviceName();

                       // Skip joysticks in the allowlist
                       static const std::array<std::string, 1> peripheralAllowlist = {
                           "virtual-remote",
                       };
                       if (std::find_if(peripheralAllowlist.begin(), peripheralAllowlist.end(),
                                        [&joystickName](const std::string& allowedJoystick) {
                                          return allowedJoystick == joystickName;
                                        }) != peripheralAllowlist.end())
                       {
                         return false;
                       }

                       // Require at least one dash
                       if (std::find_if(joystickName.begin(), joystickName.end(),
                                        [](char c) { return c == '-'; }) == joystickName.end())
                       {
                         return false;
                       }

                       // Require all lowercase letters or dashes
                       if (std::find_if(joystickName.begin(), joystickName.end(),
                                        [](char c)
                                        {
                                          const bool isLowercase = ('a' <= c && c <= 'z');
                                          const bool isDash = (c == '-');
                                          return !(isLowercase || isDash);
                                        }) != joystickName.end())
                       {
                         return false;
                       }

                       // Joystick matches the pattern, remove it
                       return true;
                     }),
      joysticks.end());

  // Move joysticks that haven't provided input yet to the end
  std::stable_sort(joysticks.begin(), joysticks.end(),
                   [](const PERIPHERALS::PeripheralPtr& lhs, const PERIPHERALS::PeripheralPtr& rhs)
                   {
                     if (lhs->LastActive().IsValid() && !rhs->LastActive().IsValid())
                       return true;
                     if (!lhs->LastActive().IsValid() && rhs->LastActive().IsValid())
                       return false;

                     return false;
                   });

  // Update agent controllers
  ProcessAgentControllers(joysticks, inputHandlingLock);

  // Update expired joysticks
  UpdateExpiredJoysticks(joysticks, inputHandlingLock);
}

void CAgentInput::ProcessKeyboard()
{
  if (m_bHasKeyboard && m_gameClient->Input().SupportsKeyboard() &&
      !m_gameClient->Input().IsKeyboardOpen())
  {
    PERIPHERALS::PeripheralVector keyboards;
    m_peripheralManager.GetPeripheralsWithFeature(keyboards, PERIPHERALS::FEATURE_KEYBOARD);
    if (!keyboards.empty())
    {
      CControllerTree controllers = m_gameClient->Input().GetActiveControllerTree();

      auto it = std::find_if(controllers.GetPorts().begin(), controllers.GetPorts().end(),
                             [](const CPortNode& port)
                             { return port.GetPortType() == PORT_TYPE::KEYBOARD; });

      PERIPHERALS::PeripheralPtr keyboard = std::move(keyboards.at(0));
      m_gameClient->Input().OpenKeyboard(it->GetActiveController().GetController(), keyboard);

      SetChanged(true);
    }
  }
}

void CAgentInput::ProcessMouse()
{
  if (m_bHasMouse && m_gameClient->Input().SupportsMouse() && !m_gameClient->Input().IsMouseOpen())
  {
    PERIPHERALS::PeripheralVector mice;
    m_peripheralManager.GetPeripheralsWithFeature(mice, PERIPHERALS::FEATURE_MOUSE);
    if (!mice.empty())
    {
      CControllerTree controllers = m_gameClient->Input().GetActiveControllerTree();

      auto it = std::find_if(controllers.GetPorts().begin(), controllers.GetPorts().end(),
                             [](const CPortNode& port)
                             { return port.GetPortType() == PORT_TYPE::MOUSE; });

      PERIPHERALS::PeripheralPtr mouse = std::move(mice.at(0));
      m_gameClient->Input().OpenMouse(it->GetActiveController().GetController(), mouse);

      SetChanged(true);
    }
  }
}

void CAgentInput::ProcessAgentControllers(const PERIPHERALS::PeripheralVector& joysticks,
                                          PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  std::lock_guard<std::mutex> lock(m_controllerMutex);

  // Handle new and existing controllers
  for (const auto& joystick : joysticks)
  {
    auto it = std::find_if(m_controllers.begin(), m_controllers.end(),
                           [&joystick](const std::shared_ptr<CAgentController>& controller)
                           { return controller->GetPeripheralLocation() == joystick->Location(); });

    if (it == m_controllers.end())
    {
      // Handle new controller
      m_controllers.emplace_back(std::make_shared<CAgentController>(*this, joystick));
      SetChanged(true);

      // Assign the agent a free port
      std::vector<std::string> inputPorts = GetInputPorts();
      for (const std::string& portAddress : inputPorts)
      {
        if (std::find_if(m_controllers.begin(), m_controllers.end(),
                         [&portAddress](const std::shared_ptr<CAgentController>& controller) {
                           return controller->GetPortAddress() == portAddress;
                         }) == m_controllers.end())
        {
          m_controllers.back()->SetPortAddress(portAddress);
          break;
        }
      }
    }
    else
    {
      CAgentController& agentController = **it;

      // Check if appearance has changed
      ControllerPtr oldController = agentController.GetController();
      ControllerPtr newController = joystick->ControllerProfile();

      std::string oldControllerId = oldController ? oldController->ID() : "";
      std::string newControllerId = newController ? newController->ID() : "";

      if (oldControllerId != newControllerId)
      {
        if (!inputHandlingLock)
          inputHandlingLock = m_peripheralManager.RegisterEventLock();

        // Reinitialize agent's new controller
        agentController.Deinitialize();
        agentController.Initialize();

        SetChanged(true);
      }
    }
  }

  // Remove expired controllers
  std::vector<std::string> expiredJoysticks;
  for (const auto& agentController : m_controllers)
  {
    auto it =
        std::find_if(joysticks.begin(), joysticks.end(),
                     [&agentController](const PERIPHERALS::PeripheralPtr& joystick)
                     { return agentController->GetPeripheralLocation() == joystick->Location(); });

    if (it == joysticks.end())
      expiredJoysticks.emplace_back(agentController->GetPeripheralLocation());
  }
  for (const std::string& expiredJoystick : expiredJoysticks)
  {
    auto it = std::find_if(m_controllers.begin(), m_controllers.end(),
                           [&expiredJoystick](const std::shared_ptr<CAgentController>& controller)
                           { return controller->GetPeripheralLocation() == expiredJoystick; });
    if (it != m_controllers.end())
    {
      if (!inputHandlingLock)
        inputHandlingLock = m_peripheralManager.RegisterEventLock();

      // Deinitialize agent
      (*it)->Deinitialize();

      // Remove from list
      m_controllers.erase(it);

      SetChanged(true);
    }
  }
}

void CAgentInput::UpdateExpiredJoysticks(const PERIPHERALS::PeripheralVector& joysticks,
                                         PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  // Make a copy - expired joysticks are removed from m_portMap
  PortMap portMapCopy = m_portMap;

  for (const auto& [inputProvider, gameClientJoystick] : portMapCopy)
  {
    // Structured binding cannot be captured, so make a copy
    JOYSTICK::IInputProvider* const inputProviderCopy = inputProvider;

    // Search peripheral vector for input provider
    auto it2 = std::find_if(joysticks.begin(), joysticks.end(),
                            [inputProviderCopy](const PERIPHERALS::PeripheralPtr& joystick)
                            {
                              // Upcast peripheral to input interface
                              JOYSTICK::IInputProvider* peripheralInput = joystick.get();

                              // Compare
                              return inputProviderCopy == peripheralInput;
                            });

    // If peripheral wasn't found, then it was disconnected
    const bool bDisconnected = (it2 == joysticks.end());

    // Erase joystick from port map if its peripheral becomes disconnected
    if (bDisconnected)
    {
      // Must use nullptr because peripheral has likely fallen out of scope,
      // destroying the object
      gameClientJoystick->UnregisterInput(nullptr);

      if (!inputHandlingLock)
        inputHandlingLock = m_peripheralManager.RegisterEventLock();
      m_portMap.erase(inputProvider);

      SetChanged(true);
    }
  }
}

void CAgentInput::UpdateConnectedJoysticks(const PERIPHERALS::PeripheralVector& joysticks,
                                           const PortMap& newPortMap,
                                           PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  for (auto& peripheralJoystick : joysticks)
  {
    // Upcast peripheral to input interface
    JOYSTICK::IInputProvider* inputProvider = peripheralJoystick.get();

    // Get connection states
    auto itConnectedPort = newPortMap.find(inputProvider);
    auto itDisconnectedPort = m_portMap.find(inputProvider);

    // Get possibly connected joystick
    std::shared_ptr<CGameClientJoystick> newJoystick;
    if (itConnectedPort != newPortMap.end())
      newJoystick = itConnectedPort->second;

    // Get possibly disconnected joystick
    std::shared_ptr<CGameClientJoystick> oldJoystick;
    if (itDisconnectedPort != m_portMap.end())
      oldJoystick = itDisconnectedPort->second;

    // Check for a change in joysticks
    if (oldJoystick != newJoystick)
    {
      // Unregister old input handler
      if (oldJoystick != nullptr)
      {
        oldJoystick->UnregisterInput(inputProvider);

        if (!inputHandlingLock)
          inputHandlingLock = m_peripheralManager.RegisterEventLock();
        m_portMap.erase(itDisconnectedPort);

        SetChanged(true);
      }

      // Register new handler
      if (newJoystick != nullptr)
      {
        newJoystick->RegisterInput(inputProvider);

        m_portMap[inputProvider] = std::move(newJoystick);

        SetChanged(true);
      }
    }
  }
}

CAgentInput::PortMap CAgentInput::MapJoysticks(
    std::vector<std::shared_ptr<CAgentController>> controllers,
    const JoystickMap& gameClientjoysticks,
    int playerLimit)
{
  PortMap result;

  // Loop through the agents and assign ports
  for (const std::shared_ptr<CAgentController>& controller : controllers)
  {
    PERIPHERALS::PeripheralPtr joystick = controller->GetPeripheral();
    const std::string& portAddress = controller->GetPortAddress();

    // Upcast joystick to input provider
    JOYSTICK::IInputProvider* inputProvider = joystick.get();

    /*! @todo Check that port is under the player limit */
    auto it = gameClientjoysticks.find(portAddress);
    if (it != gameClientjoysticks.end())
    {
      // Map joystick
      result[inputProvider] = it->second;
    }
  }

  return result;
}
