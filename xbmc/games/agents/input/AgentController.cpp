/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentController.h"

#include "AgentInput.h"
#include "AgentJoystick.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace GAME;

CAgentController::CAgentController(CAgentInput& agentInput, PERIPHERALS::PeripheralPtr peripheral)
  : m_agentInput(agentInput),
    m_peripheral(std::move(peripheral)),
    m_joystick(std::make_unique<CAgentJoystick>(m_peripheral))
{
  Initialize();
}

CAgentController::~CAgentController()
{
  Deinitialize();
}

void CAgentController::Initialize()
{
  m_joystick->Initialize();
}

void CAgentController::Deinitialize()
{
  m_joystick->Deinitialize();
}

std::string CAgentController::GetPeripheralName() const
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

const std::string& CAgentController::GetPeripheralLocation() const
{
  return m_peripheral->Location();
}

ControllerPtr CAgentController::GetController() const
{
  // Use joystick controller if joystick is initialized
  ControllerPtr controller = m_joystick->Appearance();
  if (controller)
    return controller;

  // Use peripheral controller if joystick is deinitialized
  return m_peripheral->ControllerProfile();
}

CDateTime CAgentController::LastActive() const
{
  return m_peripheral->LastActive();
}

float CAgentController::GetActivation() const
{
  return m_joystick->GetActivation();
}

void CAgentController::SetPortAddress(std::string portAddress)
{
  m_portAddress = std::move(portAddress);
}

void CAgentController::OnPortIncrease()
{
  std::vector<std::string> ports = m_agentInput.GetInputPorts();
  if (ports.empty())
    return;

  std::string oldPortAddress = m_portAddress;

  for (auto it = ports.begin(); it != ports.end(); ++it)
  {
    if (*it == m_portAddress)
    {
      ++it;
      if (it == ports.end())
        m_portAddress = ports.back();
      else
        m_portAddress = *it;
      break;
    }

    if (*it > m_portAddress)
    {
      m_portAddress = *it;
      break;
    }
  }

  if (m_portAddress != oldPortAddress)
    m_agentInput.Refresh();
}

void CAgentController::OnPortDecrease()
{
  std::vector<std::string> ports = m_agentInput.GetInputPorts();
  if (ports.empty())
    return;

  std::string oldPortAddress = m_portAddress;

  for (auto it = ports.rbegin(); it != ports.rend(); ++it)
  {
    if (*it == m_portAddress)
    {
      ++it;
      if (it == ports.rend())
        m_portAddress.clear();
      else
        m_portAddress = *it;
      break;
    }

    if (*it < m_portAddress)
    {
      m_portAddress = *it;
      break;
    }
  }

  if (m_portAddress != oldPortAddress)
    m_agentInput.Refresh();
}
