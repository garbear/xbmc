/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "AgentController.h"
#include "games/GameTypes.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"
#include "peripherals/PeripheralTypes.h"
#include "utils/Observer.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

class CInputManager;

namespace PERIPHERALS
{
class CPeripherals;
} // namespace PERIPHERALS

namespace KODI
{
namespace JOYSTICK
{
class IInputProvider;
}
namespace KEYBOARD
{
class IKeyboardInputProvider;
}
namespace MOUSE
{
class IMouseInputProvider;
}

namespace GAME
{
class CGameClient;
class CGameClientJoystick;

/*!
 * \ingroup games
 *
 * \brief Class to manage input for game-playing agents
 */
class CAgentInput : public Observer, KEYBOARD::IKeyboardDriverHandler, MOUSE::IMouseDriverHandler
{
public:
  CAgentInput(PERIPHERALS::CPeripherals& peripheralManager, CInputManager& inputManager);

  virtual ~CAgentInput();

  // Implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

  // Implementation of IKeyboardDriverHandler
  bool OnKeyPress(const CKey& key) override;
  void OnKeyRelease(const CKey& key) override {}

  // Implementation of IMouseDriverHandler
  bool OnPosition(int x, int y) override;
  bool OnButtonPress(MOUSE::BUTTON_ID button) override;
  void OnButtonRelease(MOUSE::BUTTON_ID button) override {}

  // Public interface
  /*
  std::vector<std::shared_ptr<const CAgentController>> GetControllers() const;
  float GetControllerActivation(const std::string& peripheralLocation) const;
  std::vector<std::string> GetGameInputPorts() const;
  float GetGamePortActivation(const std::string& address) const;
  */

private:
  using PeripheralLocation = std::string;

  // Internal helpers
  void Refresh();
  void ProcessAgentControllers(const PERIPHERALS::PeripheralVector& peripherals);

  // Construction parameters
  PERIPHERALS::CPeripherals& m_peripheralManager;
  CInputManager& m_inputManager;

  // State parameters
  bool m_bHasKeyboard = false;
  bool m_bHasMouse = false;
  int m_initialMouseX{-1};
  int m_initialMouseY{-1};

  // Input parameters
  std::vector<std::shared_ptr<CAgentController>> m_controllers;
};
} // namespace GAME
} // namespace KODI
