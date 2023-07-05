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

#include <string>

namespace KODI
{
namespace GAME
{

class CGameAgent
{
public:
  CGameAgent(PERIPHERALS::PeripheralPtr peripheral);

  ~CGameAgent();

  PERIPHERALS::PeripheralPtr GetPeripheral() const { return m_peripheral; }
  std::string GetPeripheralName() const;
  std::string GetPeripheralLocation() const;
  ControllerPtr GetController() const;
  CDateTime LastActive() const;

private:
  // Construction parameters
  const PERIPHERALS::PeripheralPtr m_peripheral;
};

} // namespace GAME
} // namespace KODI
