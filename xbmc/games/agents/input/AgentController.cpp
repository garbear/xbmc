/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentController.h"

#include "AgentControllerXML.h"
#include "AgentJoystick.h"
#include "AgentKeyboard.h"
#include "AgentMouse.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <cstdio>

using namespace KODI;
using namespace GAME;

CAgentController::CAgentController()
{
  ClearProperties();
}

CAgentController::CAgentController(PERIPHERALS::PeripheralPtr peripheral)
{
  ClearProperties();

  AttachPeripheral(std::move(peripheral));

  Initialize();
}

CAgentController::~CAgentController()
{
  Deinitialize();

  m_joystick.reset();
  m_keyboard.reset();
  m_mouse.reset();
  m_peripheral.reset();
}

void CAgentController::ClearProperties()
{
  m_controllerId = 0;
  m_peripheralType = PERIPHERALS::PERIPHERAL_UNKNOWN;
  m_peripheralBus = PERIPHERALS::PERIPHERAL_BUS_UNKNOWN;
  std::fill(m_peripheralVid.begin(), m_peripheralVid.end(), 0);
  std::fill(m_peripheralPid.begin(), m_peripheralPid.end(), 0);
  m_peripheralLastLocation.clear();
  m_peripheralOrdinal = 0;
  m_joystickName.clear();
  m_joystickProvider.clear();
  m_joystickButtons = 0;
  m_joystickHats = 0;
  m_joystickAxes = 0;
  m_joystickRequestedPort = PERIPHERALS::JOYSTICK_NO_PORT_REQUESTED;
  m_controllerAppearance.clear();
  m_leftDeadzone = 0.0f;
  m_rightDeadzone = 0.0f;
  m_lastActiveUtc.SetValid(false);
}

void CAgentController::AttachPeripheral(PERIPHERALS::PeripheralPtr peripheral)
{
  switch (peripheral->Type())
  {
    case PERIPHERALS::PERIPHERAL_JOYSTICK:
    {
      // Set controller parameters
      InitControllerParameters(peripheral->ControllerProfile());

      // Set peripheral parameters
      InitPeripheralParameters(*peripheral);

      // Set joystick parameters
      InitJoystickParameters(static_cast<const PERIPHERALS::CPeripheralJoystick&>(*peripheral));

      m_joystick = std::make_unique<CAgentJoystick>(peripheral);

      break;
    }
    case PERIPHERALS::PERIPHERAL_KEYBOARD:
    {
      // Set controller parameters
      InitControllerParameters(peripheral->ControllerProfile());

      // Set peripheral parameters
      InitPeripheralParameters(*peripheral);

      m_keyboard = std::make_unique<CAgentKeyboard>(peripheral);

      break;
    }
    case PERIPHERALS::PERIPHERAL_MOUSE:
    {
      // Set controller parameters
      InitControllerParameters(peripheral->ControllerProfile());

      // Set peripheral parameters
      InitPeripheralParameters(*peripheral);

      m_mouse = std::make_unique<CAgentMouse>(peripheral);

      break;
    }
    default:
      break;
  }

  m_peripheral = std::move(peripheral);
}

void CAgentController::DetachPeripheral()
{
  // Reset input
  m_joystick.reset();
  m_keyboard.reset();
  m_mouse.reset();

  // Reset peripheral
  m_peripheral.reset();
}

void CAgentController::Initialize()
{
  if (m_joystick)
    m_joystick->Initialize();
  if (m_keyboard)
    m_keyboard->Initialize();
  if (m_mouse)
    m_mouse->Initialize();
}

void CAgentController::Deinitialize()
{
  if (m_mouse)
    m_mouse->Deinitialize();
  if (m_keyboard)
    m_keyboard->Deinitialize();
  if (m_joystick)
    m_joystick->Deinitialize();
}

std::string CAgentController::GetControllerName() const
{
  std::string deviceName;

  if (!m_joystickName.empty())
    deviceName = m_joystickName;
  else if (m_peripheral)
    deviceName = m_peripheral->DeviceName();

  // Handle unknown device name
  if (deviceName.empty() && m_peripheral)
  {
    ControllerPtr controller = m_peripheral->ControllerProfile();
    if (controller)
      deviceName = controller->Layout().Label();
  }

  return deviceName;
}

const std::string& CAgentController::GetPeripheralLocation() const
{
  if (m_peripheral)
    return m_peripheral->Location();

  static const std::string empty;
  return empty;
}

ControllerPtr CAgentController::GetController() const
{
  // Use joystick controller if joystick is initialized
  if (m_joystick)
  {
    ControllerPtr controller = m_joystick->Appearance();
    if (controller)
      return controller;
  }

  // Use keyboard controller if keyboard is initialized
  if (m_keyboard)
  {
    ControllerPtr controller = m_keyboard->Appearance();
    if (controller)
      return controller;
  }

  // Use mouse controller if mouse is initialized
  if (m_mouse)
  {
    ControllerPtr controller = m_mouse->Appearance();
    if (controller)
      return controller;
  }

  // Use peripheral controller if joystick is deinitialized
  if (m_peripheral)
    return m_peripheral->ControllerProfile();

  return {};
}

CDateTime CAgentController::LastActiveUTC() const
{
  if (m_peripheral)
    return m_peripheral->LastActive().GetAsUTCDateTime();
  else if (m_lastActiveUtc.IsValid())
    return m_lastActiveUtc;

  return {};
}

float CAgentController::GetActivation() const
{
  // Return the maximum activation of all joystick, keyboard and mice input providers
  float activation = 0.0f;

  if (m_joystick)
    activation = std::max(activation, m_joystick->GetActivation());
  if (m_keyboard)
    activation = std::max(activation, m_keyboard->GetActivation());
  if (m_mouse)
    activation = std::max(activation, m_mouse->GetActivation());

  return activation;
}

void CAgentController::ClearButtonState()
{
  if (m_joystick)
    m_joystick->ClearButtonState();
  if (m_keyboard)
    m_keyboard->ClearButtonState();
  if (m_mouse)
    m_mouse->ClearButtonState();
}

bool CAgentController::IsConnected() const
{
  return static_cast<bool>(m_peripheral);
}

std::string CAgentController::GetPeripheralVIDHex() const
{
  return StringUtils::Format("{:02X}{:02X}", m_peripheralVid[0], m_peripheralVid[1]);
}

std::string CAgentController::GetPeripheralPIDHex() const
{
  return StringUtils::Format("{:02X}{:02X}", m_peripheralPid[0], m_peripheralPid[1]);
}

void CAgentController::SetPeripheralVIDHex(const std::string& vid)
{
  m_peripheralVid = ParseUsbId(vid);
}

void CAgentController::SetPeripheralPIDHex(const std::string& pid)
{
  m_peripheralPid = ParseUsbId(pid);
}

void CAgentController::InitControllerParameters(const ControllerPtr& controller)
{
  if (controller)
    m_controllerAppearance = controller->ID();
}

void CAgentController::InitPeripheralParameters(const PERIPHERALS::CPeripheral& peripheral)
{
  m_peripheralType = peripheral.Type();
  m_peripheralBus = peripheral.GetBusType();
  if (peripheral.VendorId() > 0)
  {
    m_peripheralVid[0] = (peripheral.VendorId() >> 8) & 0xff;
    m_peripheralVid[1] = peripheral.VendorId() & 0xff;
  }
  if (peripheral.ProductId() > 0)
  {
    m_peripheralPid[0] = (peripheral.ProductId() >> 8) & 0xff;
    m_peripheralPid[1] = peripheral.ProductId() & 0xff;
  }
  m_peripheralLastLocation = peripheral.Location();
  if (peripheral.LastActive().IsValid())
    m_lastActiveUtc = peripheral.LastActive().GetAsUTCDateTime();
}

void CAgentController::InitJoystickParameters(
    const PERIPHERALS::CPeripheralJoystick& peripheralJoystick)
{
  m_joystickName = peripheralJoystick.DeviceName();
  m_joystickProvider = peripheralJoystick.Provider();
  m_joystickButtons = peripheralJoystick.ButtonCount();
  m_joystickHats = peripheralJoystick.HatCount();
  m_joystickAxes = peripheralJoystick.AxisCount();
  m_joystickRequestedPort = peripheralJoystick.RequestedPort();
}

bool CAgentController::Serialize(tinyxml2::XMLElement& controllerElement) const
{
  return CAgentControllerXML::SerializeController(controllerElement, *this);
}

bool CAgentController::Deserialize(const tinyxml2::XMLElement& controllerElement)
{
  return CAgentControllerXML::DeserializeController(controllerElement, *this);
}

CAgentController::USB_ID CAgentController::ParseUsbId(const std::string& strUsbId)
{
  if (strUsbId.length() == 4)
  {
    USB_ID usbId{};
    unsigned int high;
    unsigned int low;
    int result = std::sscanf(strUsbId.c_str(), "%02x%02x", &high, &low);
    if (result == 2)
    {
      usbId[0] = static_cast<uint8_t>(high);
      usbId[1] = static_cast<uint8_t>(low);
    }
    return usbId;
  }

  return {};
}
