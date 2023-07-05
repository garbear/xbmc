/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "XBDateTime.h"
#include "games/controllers/ControllerTypes.h"
#include "peripherals/PeripheralTypes.h"

#include <memory>
#include <string>

namespace KODI
{
namespace GAME
{
class CGameAgentJoystick;

class CGameAgent
{
public:
  CGameAgent(PERIPHERALS::PeripheralPtr peripheral);

  ~CGameAgent();

  void Initialize();
  void Deinitialize();

  PERIPHERALS::PeripheralPtr GetPeripheral() const { return m_peripheral; }
  std::string GetPeripheralName() const;
  std::string GetPeripheralLocation() const;
  ControllerPtr GetController() const;
  CDateTime LastActive() const;
  float GetActivation() const;

private:
  // Construction parameters
  const PERIPHERALS::PeripheralPtr m_peripheral;

  // Input parameters
  std::unique_ptr<CGameAgentJoystick> m_joystick;
};

} // namespace GAME
} // namespace KODI
