/*
 *      Copyright (C) 2017 Team Kodi
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

#include "PlayerManager.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"

using namespace KODI;
using namespace GAME;

CPlayerManager::CPlayerManager(PERIPHERALS::CPeripherals& peripheralManager,
                               CInputManager& inputManager)
    : m_peripheralManager(peripheralManager)
    , m_inputManager(inputManager)
{
  m_peripheralManager.RegisterObserver(this);
  m_inputManager.RegisterKeyboardDriverHandler(this);
  m_inputManager.RegisterMouseDriverHandler(this);
}

CPlayerManager::~CPlayerManager()
{
  m_inputManager.UnregisterMouseDriverHandler(this);
  m_inputManager.UnregisterKeyboardDriverHandler(this);
  m_peripheralManager.UnregisterObserver(this);
}

void CPlayerManager::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessagePeripheralsChanged:
  {
    OnJoystickEvent();
    break;
  }
  default:
    break;
  }
}

bool CPlayerManager::OnKeyPress(const CKey& key)
{
  OnKeyboardAction();
  return false;
}

void CPlayerManager::OnKeyRelease(const CKey& key)
{
  OnKeyboardAction();
}

bool CPlayerManager::OnPosition(int x, int y)
{
  OnMouseAction();
  return false;
}

bool CPlayerManager::OnButtonPress(MOUSE::BUTTON_ID button)
{
  OnMouseAction();
  return false;
}

void CPlayerManager::OnButtonRelease(MOUSE::BUTTON_ID button)
{
  OnMouseAction();
}

void CPlayerManager::OnJoystickEvent()
{
  //! @todo
  using namespace PERIPHERALS;

  PeripheralVector peripherals;
  m_peripheralManager.GetPeripheralsWithFeature(peripherals, FEATURE_JOYSTICK);
}

void CPlayerManager::OnKeyboardAction()
{
  if (!m_bHasKeyboard)
  {
    m_bHasKeyboard = true;

    //! @todo Notify of state update
    using namespace PERIPHERALS;

    PeripheralVector peripherals;
    m_peripheralManager.GetPeripheralsWithFeature(peripherals, FEATURE_KEYBOARD);
  }
}

void CPlayerManager::OnMouseAction()
{
  if (!m_bHasMouse)
  {
    m_bHasMouse = true;

    //! @todo Notify of state update
    using namespace PERIPHERALS;

    PeripheralVector peripherals;
    m_peripheralManager.GetPeripheralsWithFeature(peripherals, FEATURE_MOUSE);
  }
}
