/*
*  Copyright (C) 2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include "GameAgentJoystick.h"

#include "games/controllers/Controller.h"
#include "games/controllers/input/ControllerActivity.h"
#include "input/joysticks/interfaces/IInputProvider.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace GAME;

CGameAgentJoystick::CGameAgentJoystick(PERIPHERALS::PeripheralPtr peripheral)
  : m_peripheral(std::move(peripheral)),
    m_controllerActivity(std::make_unique<CControllerActivity>())
{
}

CGameAgentJoystick::~CGameAgentJoystick() = default;

void CGameAgentJoystick::Initialize()
{
  // Upcast peripheral to input interface
  JOYSTICK::IInputProvider* inputProvider = m_peripheral.get();

  // Register input handler to silently observe all input
  inputProvider->RegisterInputHandler(this, true);
}

void CGameAgentJoystick::Deinitialize()
{
  // Upcast peripheral to input interface
  JOYSTICK::IInputProvider* inputProvider = m_peripheral.get();

  // Unregister input handler
  inputProvider->UnregisterInputHandler(this);
}

float CGameAgentJoystick::GetActivation() const
{
  return m_controllerActivity->GetActivity();
}

std::string CGameAgentJoystick::ControllerID(void) const
{
  ControllerPtr controller = m_peripheral->ControllerProfile();
  if (controller)
    return controller->ID();

  return "";
}

bool CGameAgentJoystick::HasFeature(const std::string& feature) const
{
  return true; // Capture input for all features
}

bool CGameAgentJoystick::AcceptsInput(const std::string& feature) const
{
  return true; // Accept input for all features
}

bool CGameAgentJoystick::OnButtonPress(const std::string& feature, bool bPressed)
{
  m_controllerActivity->OnButtonPress(bPressed);
  return true;
}

void CGameAgentJoystick::OnButtonHold(const std::string& feature, unsigned int holdTimeMs)
{
  m_controllerActivity->OnButtonPress(true);
}

bool CGameAgentJoystick::OnButtonMotion(const std::string& feature,
                                        float magnitude,
                                        unsigned int motionTimeMs)
{
  m_controllerActivity->OnButtonMotion(magnitude);
  return true;
}

bool CGameAgentJoystick::OnAnalogStickMotion(const std::string& feature,
                                             float x,
                                             float y,
                                             unsigned int motionTimeMs)
{
  m_controllerActivity->OnAnalogStickMotion(x, y);
  return true;
}

bool CGameAgentJoystick::OnWheelMotion(const std::string& feature,
                                       float position,
                                       unsigned int motionTimeMs)
{
  m_controllerActivity->OnWheelMotion(position);
  return true;
}

bool CGameAgentJoystick::OnThrottleMotion(const std::string& feature,
                                          float position,
                                          unsigned int motionTimeMs)
{
  m_controllerActivity->OnThrottleMotion(position);
  return true;
}

void CGameAgentJoystick::OnInputFrame()
{
  m_controllerActivity->OnInputFrame();
}
