/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroEngineJoystick.h"

#include "IRetroEngineJoystickHandler.h"
#include "games/controllers/Controller.h"
#include "games/controllers/input/ControllerState.h"
#include "input/joysticks/interfaces/IInputProvider.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CRetroEngineJoystick::CRetroEngineJoystick(PERIPHERALS::PeripheralPtr peripheral,
                                           GAME::ControllerPtr controller,
                                           IRetroEngineJoystickHandler& joystickHandler)
  : m_peripheral(std::move(peripheral)),
    m_controller(std::move(controller)),
    m_joystickHandler(joystickHandler),
    m_controllerState(std::make_unique<GAME::CControllerState>(*m_controller))
{
  // Upcast peripheral to input interface
  JOYSTICK::IInputProvider* inputProvider = m_peripheral.get();

  // Register input handler to silently observe all input
  inputProvider->RegisterInputHandler(this, true);
}

CRetroEngineJoystick::~CRetroEngineJoystick()
{
  // Upcast peripheral to input interface
  JOYSTICK::IInputProvider* inputProvider = m_peripheral.get();

  // Unregister input handler
  inputProvider->UnregisterInputHandler(this);
}

std::string CRetroEngineJoystick::ControllerID(void) const
{
  return m_controller->ID();
}

bool CRetroEngineJoystick::HasFeature(const std::string& feature) const
{
  return true; // Capture input for all features
}

bool CRetroEngineJoystick::AcceptsInput(const std::string& feature) const
{
  return true; // Accept input for all features
}

bool CRetroEngineJoystick::OnButtonPress(const std::string& feature, bool bPressed)
{
  m_controllerState->SetDigitalButton(feature, bPressed);
  return true;
}

bool CRetroEngineJoystick::OnButtonMotion(const std::string& feature,
                                          float magnitude,
                                          unsigned int motionTimeMs)
{
  m_controllerState->SetAnalogButton(feature, magnitude);
  return true;
}

bool CRetroEngineJoystick::OnAnalogStickMotion(const std::string& feature,
                                               float x,
                                               float y,
                                               unsigned int motionTimeMs)
{
  m_controllerState->SetAnalogStick(feature, {x, y});
  return true;
}

bool CRetroEngineJoystick::OnAccelerometerMotion(const std::string& feature,
                                                 float x,
                                                 float y,
                                                 float z)
{
  m_controllerState->SetAccelerometer(feature, {x, y, z});
  return true;
}

bool CRetroEngineJoystick::OnWheelMotion(const std::string& feature,
                                         float position,
                                         unsigned int motionTimeMs)
{
  m_controllerState->SetWheel(feature, position);
  return true;
}

bool CRetroEngineJoystick::OnThrottleMotion(const std::string& feature,
                                            float position,
                                            unsigned int motionTimeMs)
{
  m_controllerState->SetThrottle(feature, position);
  return true;
}

void CRetroEngineJoystick::OnInputFrame()
{
  return m_joystickHandler.OnInputFrame(*m_peripheral, *m_controllerState);
}
