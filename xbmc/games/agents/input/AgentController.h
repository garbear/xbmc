/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "XBDateTime.h"
#include "games/agents/input/IAgentJoystickHandler.h"
#include "games/controllers/ControllerTypes.h"
#include "peripherals/PeripheralTypes.h"

#include <memory>
#include <string>

namespace KODI
{
namespace GAME
{
class CAgentInput;
class CAgentJoystick;

/*!
 * \ingroup games
 *
 * \brief Class to represent the controller of a game player (a.k.a. agent)
 *
 * The term "agent" is used to distinguish game players from the myriad of other
 * uses of the term "player" in Kodi, such as media players and player cores.
 */
class CAgentController : public IAgentJoystickHandler
{
public:
  CAgentController(CAgentInput& agentInput, PERIPHERALS::PeripheralPtr peripheral);
  ~CAgentController();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  // Input properties
  PERIPHERALS::PeripheralPtr GetPeripheral() const { return m_peripheral; }
  std::string GetPeripheralName() const;
  const std::string& GetPeripheralLocation() const;
  ControllerPtr GetController() const;
  CDateTime LastActive() const;
  float GetActivation() const;
  const std::string& GetPortAddress() const { return m_portAddress; }

  // Input mutators
  void SetPortAddress(std::string portAddress);

  // Implementation of IGameAgentJoystickHandler
  void OnPortIncrease() override;
  void OnPortDecrease() override;

private:
  // Construction parameters
  CAgentInput& m_agentInput;
  const PERIPHERALS::PeripheralPtr m_peripheral;

  // Input parameters
  std::unique_ptr<CAgentJoystick> m_joystick;
  std::string m_portAddress;
};

} // namespace GAME
} // namespace KODI
