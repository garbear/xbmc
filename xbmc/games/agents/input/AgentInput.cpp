/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentInput.h"

#include "AgentController.h"
#include "AgentControllerMap.h"
#include "AgentInputMap.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/input/GameClientJoystick.h"
#include "games/controllers/Controller.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "peripherals/events/EventLockHandle.h"
#include "utils/log.h"

#include <array>

using namespace KODI;
using namespace GAME;

CAgentInput::CAgentInput(PERIPHERALS::CPeripherals& peripheralManager, CInputManager& inputManager)
  : m_peripheralManager(peripheralManager),
    m_inputManager(inputManager),
    m_inputMap(std::make_unique<CAgentInputMap>()),
    m_controllerMap(std::make_unique<CAgentControllerMap>())
{
}

CAgentInput::~CAgentInput() = default;

void CAgentInput::Initialize()
{
  // Load input map
  //! @todo Load async to not block main thread during app initialization
  m_inputMap->LoadXML();

  // Load controller map
  //! @todo Load async to not block main thread during app initialization
  m_controllerMap->LoadXML();

  // Register callbacks
  m_peripheralManager.RegisterObserver(this);
  m_inputManager.RegisterKeyboardDriverHandler(this);
  m_inputManager.RegisterMouseDriverHandler(this);
}

void CAgentInput::Deinitialize()
{
  // Unregister callbacks in reverse order
  m_inputManager.UnregisterMouseDriverHandler(this);
  m_inputManager.UnregisterKeyboardDriverHandler(this);
  m_peripheralManager.UnregisterObserver(this);

  // Clear input map
  m_inputMap->Clear();

  // Clear controller map
  m_controllerMap->Clear();
}

void CAgentInput::Start(GameClientPtr gameClient)
{
  // Initialize state
  m_gameClient = std::move(gameClient);

  // Register callbacks
  if (m_gameClient)
    m_gameClient->Input().RegisterObserver(this);

  // Perform initial refresh
  Refresh();

  // Update input map
  m_inputMap->AddGameClient(m_gameClient);
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
    m_peripheralMap.clear();
    m_disconnectedPeripherals.clear();
  }

  // Close open keyboard
  if (!m_keyboardPort.empty())
  {
    m_keyboardPort.clear();
    SetChanged(true);
  }

  // Close open mouse
  if (!m_mousePort.empty())
  {
    m_mousePort.clear();
    SetChanged(true);
  }

  // Notify observers if anything changed
  NotifyObservers(ObservableMessageAgentControllersChanged);

  // Reset state
  m_gameClient.reset();
}

void CAgentInput::Refresh()
{
  PERIPHERALS::EventLockHandlePtr inputHandlingLock;
  ProcessAgentControllers(inputHandlingLock);

  if (m_gameClient)
  {
    // Open keyboard
    if (m_bHasKeyboard)
      ProcessKeyboard();

    // Open mouse
    if (m_bHasMouse)
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
  if (!m_bHasKeyboard)
  {
    m_bHasKeyboard = true;
    ProcessKeyboard();
    NotifyObservers(ObservableMessageAgentControllersChanged);
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
      ProcessMouse();
      NotifyObservers(ObservableMessageAgentControllersChanged);
    }
  }
  return false;
}

bool CAgentInput::OnButtonPress(MOUSE::BUTTON_ID button)
{
  if (!m_bHasMouse)
  {
    m_bHasMouse = true;
    ProcessMouse();
    NotifyObservers(ObservableMessageAgentControllersChanged);
  }
  return false;
}

std::vector<std::shared_ptr<const CAgentController>> CAgentInput::GetControllers() const
{
  std::lock_guard<std::mutex> lock(m_controllerMutex);

  return m_controllerMap->GetConnectedControllers();
}

std::string CAgentInput::GetPortAddress(JOYSTICK::IInputProvider* inputProvider) const
{
  auto it = m_portMap.find(inputProvider);
  if (it != m_portMap.end())
    return it->second->GetPortAddress();

  return "";
}

std::string CAgentInput::GetKeyboardAddress(KEYBOARD::IKeyboardInputProvider* inputProvider) const
{
  auto it = m_keyboardPort.find(inputProvider);
  if (it != m_keyboardPort.end())
    return it->second;

  return "";
}

std::string CAgentInput::GetMouseAddress(MOUSE::IMouseInputProvider* inputProvider) const
{
  auto it = m_mousePort.find(inputProvider);
  if (it != m_mousePort.end())
    return it->second;

  return "";
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

float CAgentInput::GetPeripheralActivation(const std::string& peripheralLocation) const
{
  std::lock_guard<std::mutex> lock(m_controllerMutex);

  return m_controllerMap->GetPeripheralActivation(peripheralLocation);
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

  // Update expired joysticks
  UpdateExpiredJoysticks(joysticks, inputHandlingLock);

  // Perform the port mapping
  PortMap newPortMap =
      MapJoysticks(joysticks, m_gameClient->Input().GetJoystickMap(), m_currentPorts,
                   m_currentPeripherals, m_gameClient->Input().GetPlayerLimit());

  // Update connected joysticks
  std::set<PERIPHERALS::PeripheralPtr> disconnectedPeripherals;
  UpdateConnectedJoysticks(joysticks, newPortMap, inputHandlingLock, disconnectedPeripherals);

  // Rebuild peripheral map
  PeripheralMap peripheralMap;
  for (const auto& [inputProvider, joystick] : m_portMap)
    peripheralMap[joystick->GetControllerAddress()] = joystick->GetSource();

  // Log peripheral map if there were any changes
  if (peripheralMap != m_peripheralMap || disconnectedPeripherals != m_disconnectedPeripherals)
  {
    m_peripheralMap = std::move(peripheralMap);
    m_disconnectedPeripherals = std::move(disconnectedPeripherals);
    LogPeripheralMap(m_peripheralMap, m_disconnectedPeripherals);
  }
}

void CAgentInput::ProcessKeyboard()
{
  PERIPHERALS::PeripheralVector keyboards;
  m_peripheralManager.GetPeripheralsWithFeature(keyboards, PERIPHERALS::FEATURE_KEYBOARD);

  if (!keyboards.empty())
  {
    // Process keyboard input
    if (m_gameClient && m_gameClient->Input().SupportsKeyboard() &&
        !m_gameClient->Input().IsKeyboardOpen())
    {
      // Get controller in keyboard port
      const CControllerTree controllers = m_gameClient->Input().GetActiveControllerTree();
      auto it = std::find_if(controllers.GetPorts().begin(), controllers.GetPorts().end(),
                             [](const CPortNode& port)
                             { return port.GetPortType() == PORT_TYPE::KEYBOARD; });

      // Open keyboard input
      PERIPHERALS::PeripheralPtr keyboard = std::move(keyboards.at(0));
      m_gameClient->Input().OpenKeyboard(it->GetActiveController().GetController(), keyboard);

      // Save keyboard port
      m_keyboardPort[static_cast<KEYBOARD::IKeyboardInputProvider*>(keyboard.get())] =
          it->GetAddress();

      SetChanged(true);
    }
  }
}

void CAgentInput::ProcessMouse()
{
  PERIPHERALS::PeripheralVector mice;
  m_peripheralManager.GetPeripheralsWithFeature(mice, PERIPHERALS::FEATURE_MOUSE);

  if (!mice.empty())
  {
    // Process mouse input
    if (m_gameClient && m_gameClient->Input().SupportsMouse() &&
        !m_gameClient->Input().IsMouseOpen())
    {
      // Get controller in mouse port
      CControllerTree controllers = m_gameClient->Input().GetActiveControllerTree();

      auto it = std::find_if(controllers.GetPorts().begin(), controllers.GetPorts().end(),
                             [](const CPortNode& port)
                             { return port.GetPortType() == PORT_TYPE::MOUSE; });

      // Open mouse input
      PERIPHERALS::PeripheralPtr mouse = std::move(mice.at(0));
      m_gameClient->Input().OpenMouse(it->GetActiveController().GetController(), mouse);

      // Save mouse port
      m_mousePort[static_cast<MOUSE::IMouseInputProvider*>(mouse.get())] = it->GetAddress();

      SetChanged(true);
    }
  }
}

void CAgentInput::ProcessAgentControllers(PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  PERIPHERALS::PeripheralVector peripherals;
  m_peripheralManager.GetPeripheralsWithFeature(peripherals, PERIPHERALS::FEATURE_JOYSTICK);
  if (m_bHasKeyboard)
    m_peripheralManager.GetPeripheralsWithFeature(peripherals, PERIPHERALS::FEATURE_KEYBOARD);
  if (m_bHasMouse)
    m_peripheralManager.GetPeripheralsWithFeature(peripherals, PERIPHERALS::FEATURE_MOUSE);

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
                     [](const PERIPHERALS::PeripheralPtr& peripheral)
                     {
                       if (peripheral->Type() != PERIPHERALS::PERIPHERAL_JOYSTICK)
                         return false;

                       const std::string& joystickName = peripheral->DeviceName();

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

  std::vector<std::shared_ptr<const CAgentController>> connectedControllers;

  {
    std::lock_guard<std::mutex> lock(m_controllerMutex);
    connectedControllers = m_controllerMap->HandlePeripherals(peripherals);
  }

  // Sort controllers in the order:
  //
  //   - Keyboard, if game client accepts keyboard input
  //   - Mouse, if game client accepts mouse input
  //   - Joysticks, in order of last button press
  //   - Keyboard, if game client doesn't accept keyboard input
  //   - Mouse, if game client doesn't accept mouse input
  //
  std::sort(connectedControllers.begin(), connectedControllers.end(),
            [this](const std::shared_ptr<const CAgentController>& lhs,
                   const std::shared_ptr<const CAgentController>& rhs)
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
}

void CAgentInput::UpdateExpiredJoysticks(const PERIPHERALS::PeripheralVector& joysticks,
                                         PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  // Make a copy - expired joysticks are removed from m_portMap
  PortMap portMapCopy = m_portMap;

  for (const auto& [inputProvider, joystick] : portMapCopy)
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
      joystick->UnregisterInput(nullptr);

      if (!inputHandlingLock)
        inputHandlingLock = m_peripheralManager.RegisterEventLock();
      m_portMap.erase(inputProvider);

      SetChanged(true);
    }
  }
}

void CAgentInput::UpdateConnectedJoysticks(
    const PERIPHERALS::PeripheralVector& joysticks,
    const PortMap& newPortMap,
    PERIPHERALS::EventLockHandlePtr& inputHandlingLock,
    std::set<PERIPHERALS::PeripheralPtr>& disconnectedPeripherals)
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

  // Record disconnected peripherals
  for (const auto& peripheral : joysticks)
  {
    // Upcast peripheral to input interface
    JOYSTICK::IInputProvider* inputProvider = peripheral.get();

    // Check if peripheral is disconnected
    if (m_portMap.find(inputProvider) == m_portMap.end())
      disconnectedPeripherals.emplace(peripheral);
  }
}

CAgentInput::PortMap CAgentInput::MapJoysticks(
    const PERIPHERALS::PeripheralVector& peripheralJoysticks,
    const JoystickMap& gameClientjoysticks,
    CurrentPortMap& currentPorts,
    CurrentPeripheralMap& currentPeripherals,
    int playerLimit)
{
  PortMap result;

  // First, create a map of the current ports to attempt to preserve
  // player numbers
  for (const auto& [portAddress, joystick] : gameClientjoysticks)
  {
    std::string sourceLocation = joystick->GetSourceLocation();
    if (!sourceLocation.empty())
      currentPorts[portAddress] = std::move(sourceLocation);
  }

  // Allow reverse lookups by peripheral location
  for (const auto& [portAddress, sourceLocation] : currentPorts)
    currentPeripherals[sourceLocation] = portAddress;

  // Next, create a list of joystick peripherals sorted by order of last
  // button press. Joysticks without a current port are assigned in this
  // order.
  PERIPHERALS::PeripheralVector availableJoysticks = peripheralJoysticks;
  std::sort(availableJoysticks.begin(), availableJoysticks.end(),
            [](const PERIPHERALS::PeripheralPtr& lhs, const PERIPHERALS::PeripheralPtr& rhs)
            {
              if (lhs->LastActive().IsValid() && !rhs->LastActive().IsValid())
                return true;
              if (!lhs->LastActive().IsValid() && rhs->LastActive().IsValid())
                return false;

              return lhs->LastActive() > rhs->LastActive();
            });

  // Loop through the active ports and assign joysticks
  unsigned int iJoystick = 0;
  for (const auto& [portAddress, gameClientJoystick] : gameClientjoysticks)
  {
    const unsigned int joystickCount = ++iJoystick;

    // Check if we're out of joystick peripherals or over the topology limit
    if (availableJoysticks.empty() ||
        (playerLimit >= 0 && static_cast<int>(joystickCount) > playerLimit))
    {
      gameClientJoystick->ClearSource();
      continue;
    }

    PERIPHERALS::PeripheralVector::iterator itJoystick = availableJoysticks.end();

    // Attempt to preserve player numbers
    auto itCurrentPort = currentPorts.find(portAddress);
    if (itCurrentPort != currentPorts.end())
    {
      const PeripheralLocation& currentPeripheral = itCurrentPort->second;

      // Find peripheral with matching source location
      itJoystick = std::find_if(availableJoysticks.begin(), availableJoysticks.end(),
                                [&currentPeripheral](const PERIPHERALS::PeripheralPtr& joystick)
                                { return joystick->Location() == currentPeripheral; });
    }

    if (itJoystick == availableJoysticks.end())
    {
      // Get the next most recently active joystick that doesn't have a current port
      itJoystick = std::find_if(
          availableJoysticks.begin(), availableJoysticks.end(),
          [&currentPeripherals, &gameClientjoysticks](const PERIPHERALS::PeripheralPtr& joystick)
          {
            const PeripheralLocation& joystickLocation = joystick->Location();

            // If joystick doesn't have a current port, use it
            auto itPeripheral = currentPeripherals.find(joystickLocation);
            if (itPeripheral == currentPeripherals.end())
              return true;

            // Get the address of the last port this joystick was connected to
            const PortAddress& portAddress = itPeripheral->second;

            // If port is disconnected, use this joystick
            if (gameClientjoysticks.find(portAddress) == gameClientjoysticks.end())
              return true;

            return false;
          });
    }

    // If found, assign the port and remove from the lists
    if (itJoystick != availableJoysticks.end())
    {
      // Dereference iterator
      PERIPHERALS::PeripheralPtr peripheralJoystick = *itJoystick;

      // Map joystick
      MapJoystick(std::move(peripheralJoystick), gameClientJoystick, result);

      // Remove from availableJoysticks list
      availableJoysticks.erase(itJoystick);
    }
    else
    {
      // No joystick found, clear the port
      gameClientJoystick->ClearSource();
    }
  }

  return result;
}

void CAgentInput::MapJoystick(PERIPHERALS::PeripheralPtr peripheralJoystick,
                              std::shared_ptr<CGameClientJoystick> gameClientJoystick,
                              PortMap& result)
{
  // Upcast peripheral joystick to input provider
  JOYSTICK::IInputProvider* inputProvider = peripheralJoystick.get();

  // Update game joystick's source peripheral
  gameClientJoystick->SetSource(std::move(peripheralJoystick));

  // Map input provider to input handler
  result[inputProvider] = std::move(gameClientJoystick);
}

void CAgentInput::LogPeripheralMap(
    const PeripheralMap& peripheralMap,
    const std::set<PERIPHERALS::PeripheralPtr>& disconnectedPeripherals)
{
  CLog::Log(LOGDEBUG, "===== Peripheral Map =====");

  unsigned int line = 0;

  if (!peripheralMap.empty())
  {
    for (const auto& [controllerAddress, peripheral] : peripheralMap)
    {
      if (line != 0)
        CLog::Log(LOGDEBUG, "");
      CLog::Log(LOGDEBUG, "{}:", controllerAddress);
      CLog::Log(LOGDEBUG, "    {} [{}]", peripheral->Location(), peripheral->DeviceName());

      ++line;
    }
  }

  if (!disconnectedPeripherals.empty())
  {
    if (line != 0)
      CLog::Log(LOGDEBUG, "");
    CLog::Log(LOGDEBUG, "Disconnected:");

    // Sort by peripheral location
    std::map<std::string, std::string> disconnectedPeripheralMap;
    for (const auto& peripheral : disconnectedPeripherals)
      disconnectedPeripheralMap[peripheral->Location()] = peripheral->DeviceName();

    // Log location and device name for disconnected peripherals
    for (const auto& [location, deviceName] : disconnectedPeripheralMap)
      CLog::Log(LOGDEBUG, "    {} [{}]", location, deviceName);
  }

  CLog::Log(LOGDEBUG, "==========================");
}
