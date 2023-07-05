/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameAgent.h"

#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "peripherals/devices/Peripheral.h"

#include <cassert>

using namespace KODI;
using namespace GAME;

CGameAgent::CGameAgent(PERIPHERALS::PeripheralPtr peripheral) : m_peripheral(std::move(peripheral))
{
  assert(m_peripheral);
}

CGameAgent::~CGameAgent() = default;

std::string CGameAgent::GetPeripheralName() const
{
  std::string deviceName = m_peripheral->DeviceName();

  // Handle unknown device name
  if (deviceName.empty())
  {
    ControllerPtr controller = m_peripheral->ControllerProfile();
    if (controller)
      deviceName = controller->Layout().Label();
  }

  return deviceName;
}

std::string CGameAgent::GetPeripheralLocation() const
{
  return m_peripheral->Location();
}

ControllerPtr CGameAgent::GetController() const
{
  return m_peripheral->ControllerProfile();
}

CDateTime CGameAgent::LastActive() const
{
  return m_peripheral->LastActive();
}
