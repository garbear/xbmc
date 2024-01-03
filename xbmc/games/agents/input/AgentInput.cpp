/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentInput.h"

#include "AgentController.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/devices/PeripheralJoystick.h"

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

void CAgentInput::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
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
  if (!m_bHasKeyboard)
  {
    m_bHasKeyboard = true;
    Refresh();
  }
  return false;
}

bool CAgentInput::OnPosition(int x, int y)
{
  if (!m_bHasMouse)
  {
    // Only process mouse if the position has changed
    if (m_initialMouseX == -1 && m_initialMouseY == -1)
    {
      m_initialMouseX = x;
      m_initialMouseY = y;
    }

    if (m_initialMouseX != x || m_initialMouseY != y)
    {
      m_bHasMouse = true;
      Refresh();
    }
  }
  return false;
}

bool CAgentInput::OnButtonPress(MOUSE::BUTTON_ID button)
{
  if (!m_bHasMouse)
  {
    m_bHasMouse = true;
    Refresh();
  }
  return false;
}

void CAgentInput::Refresh()
{
  PERIPHERALS::PeripheralVector peripherals;
  m_peripheralManager.GetPeripheralsWithFeature(peripherals, PERIPHERALS::FEATURE_JOYSTICK);

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
  peripherals.erase(
      std::remove_if(peripherals.begin(), peripherals.end(),
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
      peripherals.end());

  if (m_bHasKeyboard)
    m_peripheralManager.GetPeripheralsWithFeature(peripherals, PERIPHERALS::FEATURE_KEYBOARD);
  if (m_bHasMouse)
    m_peripheralManager.GetPeripheralsWithFeature(peripherals, PERIPHERALS::FEATURE_MOUSE);

  ProcessAgentControllers(peripherals);
}

void CAgentInput::ProcessAgentControllers(const PERIPHERALS::PeripheralVector& peripherals)
{
  std::vector<std::shared_ptr<CAgentController>> connectedControllers;

  // Look for new controllers
  for (const PERIPHERALS::PeripheralPtr& peripheral : peripherals)
  {
    // Look up controllers with same properties physical properties. We'll
    // use ordinal and last location to judge controller uniqueness

    std::vector<CAgentController> matchingControllers;

    // If matching controller has a lastLocation of an active peripheral not
    // this peripheral, skip it

    // If matching controller has a lastLocation of this peripheral, assign it

    // Give new controller a new ordinal

    // Mark controller as connected
  }

  // Create a new agent
  CGameAgent agent;
  /*
  agent.playerNumber = 1;
  agent.name = "Mr. Man";
  agent.avatar = "resource://resource.avatar.opengameart/avatars/Blue_Enemy/01x/001.png";
  */

  // By default, assign agent the new controller
  /*
  CAgentControllerMap map{"game.controller.snes"};
  map["a"] = {
      .mapController = agentController.uniqueId,
      .mapAs = agentController.controllerProfile,
      .mapButton = "b",
  };
  */

  // Look for disconnected controllers
  for (const std::shared_ptr<CAgentController>& controller : m_controllers)
  {
    // Check if controller was marked as connected

    // If disconnected, deinitialize input
  }
}

std::string ControllerID() const
{
  return "game.controller.default";
}

int ControllerUID() const
{
  return 2;
}

bool OnButtonPress(const std::string& feature, bool bPressed)
{
  const int uniqueId = ControllerUID();

  // Get Controller ID
  for (auto agent : m_agents)
  {
    for (auto controllerMapping : agent.mappings)
    {
      if (controllerMapping.ControllerID() == agent.port.Controller())
      {
        for (auto button : mapping.buttons)
        {
          if (mapController == uniqueId)
          {
            // Get buttonmap that translates game.controller.default to "mapas" controller
            mappedFeature = "b";
            if (mappedFeature == mapButton)
            {
              // Send input to agent's port
            }
          }
        }
        for (auto [analogStick, direction] : mapping.analogSticks)
        {
        }
      }
    }
  }
}

/*
std::vector<std::shared_ptr<CAgentController>> CAgentInput::GetControllers() const
{
  std::vector<std::shared_ptr<CAgentController>> controllers;

  std::lock_guard<std::mutex> lock(m_controllerMutex);
  for (auto it : m_controllers)
    controllers.emplace_back(it.second);

  // Sort controllers in the order:
  //
  //   - Keyboard, if game client accepts keyboard input
  //   - Mouse, if game client accepts mouse input
  //   - Joysticks, in order of last button press
  //   - Keyboard, if game client doesn't accept keyboard input
  //   - Mouse, if game client doesn't accept mouse input
  //
  std::sort(controllers.begin(), controllers.end(),
            [this](const std::shared_ptr<CAgentController>& lhs,
                   const std::shared_ptr<CAgentController>& rhs)
            {
              const PERIPHERALS::PeripheralPtr& lhsPeripheral = lhs->GetPeripheral();
              const PERIPHERALS::PeripheralPtr& rhsPeripheral = rhs->GetPeripheral();

              if (m_gameClient && m_gameClient->Input().SupportsKeyboard())
              {
                if (lhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_KEYBOARD &&
                    rhsPeripheral->Type() != PERIPHERALS::PERIPHERAL_KEYBOARD)
                  return true;
                if (lhsPeripheral->Type() != PERIPHERALS::PERIPHERAL_KEYBOARD &&
                    rhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_KEYBOARD)
                  return false;
              }

              if (m_gameClient && m_gameClient->Input().SupportsMouse())
              {
                if (lhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_MOUSE &&
                    rhsPeripheral->Type() != PERIPHERALS::PERIPHERAL_MOUSE)
                  return true;
                if (lhsPeripheral->Type() != PERIPHERALS::PERIPHERAL_MOUSE &&
                    rhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_MOUSE)
                  return false;
              }

              if (lhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_JOYSTICK &&
                  rhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_JOYSTICK)
              {
                if (lhsPeripheral->LastActive().IsValid() && !rhsPeripheral->LastActive().IsValid())
                  return true;
                if (!lhsPeripheral->LastActive().IsValid() && rhsPeripheral->LastActive().IsValid())
                  return false;

                return lhsPeripheral->LastActive() > rhsPeripheral->LastActive();
              }

              if (lhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_JOYSTICK &&
                  rhsPeripheral->Type() != PERIPHERALS::PERIPHERAL_JOYSTICK)
                return true;
              if (lhsPeripheral->Type() != PERIPHERALS::PERIPHERAL_JOYSTICK &&
                  rhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_JOYSTICK)
                return false;

              if (lhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_KEYBOARD &&
                  rhsPeripheral->Type() != PERIPHERALS::PERIPHERAL_KEYBOARD)
                return true;
              if (lhsPeripheral->Type() != PERIPHERALS::PERIPHERAL_KEYBOARD &&
                  rhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_KEYBOARD)
                return false;

              return lhsPeripheral->Type() == PERIPHERALS::PERIPHERAL_MOUSE &&
                     rhsPeripheral->Type() != PERIPHERALS::PERIPHERAL_MOUSE;
            });

  return controllers;
}

float CAgentInput::GetControllerActivation(const std::string& peripheralLocation) const
{
  std::lock_guard<std::mutex> lock(m_controllerMutex);

  for (auto it : m_controllers)
  {
    const CAgentController& controller = *it.second;
    if (controller.GetPeripheralLocation() == peripheralLocation)
      return controller.GetActivation();
  }

  return 0.0f;
}

std::vector<std::string> CAgentInput::GetGameInputPorts() const
{
  std::vector<std::string> inputPorts;

  if (m_gameClient)
  {
    CControllerTree controllerTree = m_gameClient->Input().GetActiveControllerTree();
    controllerTree.GetInputPorts(inputPorts);
  }

  return inputPorts;
}

float CAgentInput::GetGamePortActivation(const std::string& portAddress) const
{
  float activation = 0.0f;

  if (m_gameClient)
    activation = m_gameClient->Input().GetPortActivation(portAddress);

  return activation;
}
*/
