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

#include "peripherals/PeripheralTypes.h"
#include "utils/Observer.h"

#include <set>

namespace PERIPHERALS
{
  class CPeripheral;
  class CPeripherals;
}

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
  class CPlayerManager : public Observable
  {
  public:
    CPlayerManager(PERIPHERALS::CPeripherals& peripheralManager);

    virtual ~CPlayerManager();

    /*!
     * \brief Send a hardware reset command for the specified input handler
     *
     * \param handler  The handler associated the user who pressed reset, or
     *                 nullptr if it's unknown who presesd reset
     */
    void HardwareReset(JOYSTICK::IInputHandler *handler = nullptr) { }

  private:
    void OnEnable();
    void OnDisable();

    // Construction parameters
    PERIPHERALS::CPeripherals &m_peripheralManager;

    bool m_bEnabled = false;

    struct SGamePlayer
    {
      JOYSTICK::IInputHandler*    handler; // Input handler for this port
      HARDWARE::IHardwareInput    *hardwareInput; // Callbacks for hardware input
      unsigned int                port;    // Port number belonging to the game client
      PERIPHERALS::PeripheralType requiredType;
      void*                       device;
      CGameClient*                gameClient;
    };
  };
}
}
