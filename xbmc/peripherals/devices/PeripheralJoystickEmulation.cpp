/*
 *      Copyright (C) 2015-2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PeripheralJoystickEmulation.h"
#include "input/keyboard/generic/GenericKeyboardJoystick.h"
#include "input/InputManager.h"

#include <sstream>

using namespace PERIPHERALS;

CPeripheralJoystickEmulation::CPeripheralJoystickEmulation(const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheral(scanResult, bus),
  m_keyboardHandler(nullptr)
{
  m_features.push_back(FEATURE_JOYSTICK);
}

CPeripheralJoystickEmulation::~CPeripheralJoystickEmulation(void)
{
  if (m_keyboardHandler)
  {
    CInputManager::GetInstance().UnregisterKeyboardHandler(m_keyboardHandler);
    delete m_keyboardHandler;
  }
}

bool CPeripheralJoystickEmulation::InitialiseFeature(const PeripheralFeature feature)
{
  bool bSuccess = false;

  if (CPeripheral::InitialiseFeature(feature))
  {
    if (feature == FEATURE_JOYSTICK)
    {
      m_keyboardHandler = new KEYBOARD::CGenericKeyboardJoystick;
      CInputManager::GetInstance().RegisterKeyboardHandler(m_keyboardHandler);
    }
    bSuccess = true;
  }

  return bSuccess;
}

void CPeripheralJoystickEmulation::RegisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler, bool bPromiscuous)
{
  m_keyboardHandler->RegisterJoystickDriverHandler(handler, bPromiscuous);
}

void CPeripheralJoystickEmulation::UnregisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler)
{
  m_keyboardHandler->UnregisterJoystickDriverHandler(handler);
}

unsigned int CPeripheralJoystickEmulation::ControllerNumber(void) const
{
  unsigned int number;
  std::istringstream str(m_strLocation);
  str >> number;
  return number;
}
