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
#pragma once

#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"
#include "peripherals/PeripheralTypes.h"
#include "utils/Observer.h"

#include <set>

class CInputManager;

namespace PERIPHERALS
{
class CPeripheral;
class CPeripherals;
} // namespace PERIPHERALS

namespace KODI
{
namespace HARDWARE
{
class IHardwareInput;
}

namespace JOYSTICK
{
class IInputHandler;
}

namespace GAME
{
class CGameClient;
class CPlayer;

/*!
 * \brief Class to manage ports opened by game clients
 */
class CPlayerManager : public Observer, KEYBOARD::IKeyboardDriverHandler, MOUSE::IMouseDriverHandler
{
public:
  CPlayerManager(PERIPHERALS::CPeripherals& peripheralManager, CInputManager& inputManager);

  virtual ~CPlayerManager();

  // Implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

  // Implementation of IKeyboardDriverHandler
  bool OnKeyPress(const CKey& key) override;
  void OnKeyRelease(const CKey& key) override;

  // Implementation of IMouseDriverHandler
  bool OnPosition(int x, int y) override;
  bool OnButtonPress(MOUSE::BUTTON_ID button) override;
  void OnButtonRelease(MOUSE::BUTTON_ID button) override;

private:
  void OnJoystickEvent();
  void OnKeyboardAction();
  void OnMouseAction();

  // Construction parameters
  PERIPHERALS::CPeripherals& m_peripheralManager;
  CInputManager& m_inputManager;

  // State parameters
  bool m_bHasKeyboard = false;
  bool m_bHasMouse = false;

  struct SGamePlayer
  {
    JOYSTICK::IInputHandler* handler;        // Input handler for this port
    HARDWARE::IHardwareInput* hardwareInput; // Callbacks for hardware input
    unsigned int port;                       // Port number belonging to the game client
    PERIPHERALS::PeripheralType requiredType;
    void* device;
    CGameClient* gameClient;
  };
};
} // namespace GAME
} // namespace KODI
