/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "input/joysticks/IJoystickButtonMap.h"
#include "peripherals/addons/PeripheralAddon.h"

#include <map>

namespace PERIPHERALS
{
  class CAddonJoystickButtonMap : public IJoystickButtonMap
  {
  public:
    CAddonJoystickButtonMap(const PeripheralAddonPtr& addon, unsigned int index);

    virtual ~CAddonJoystickButtonMap(void) { }

    // Implementation of IJoystickButtonMap
    virtual bool Load(void);
    JoystickFeatureID GetAction(const CInputPrimitive& button);
    bool GetInputPrimitive(JoystickFeatureID id, CInputPrimitive& button);
    bool GetAnalogStick(JoystickFeatureID id, int& horizIndex, bool& horizInverted,
                                              int& vertIndex,  bool& vertInverted);
    bool GetAccelerometer(JoystickFeatureID id, int& xIndex, bool& xInverted,
                                                int& yIndex, bool& yInverted,
                                                int& zIndex, bool& zInverted);

  private:
    static std::map<CInputPrimitive, JoystickFeatureID> GetActionMap(const JoystickFeatureMap& features);
    static HatDirection      ToHatDirection(JOYSTICK_DRIVER_HAT_DIRECTION driverDirection);
    static SemiAxisDirection ToSemiAxisDirection(JOYSTICK_DRIVER_SEMIAXIS_DIRECTION dir);

    const PeripheralAddonPtr m_addon;
    const unsigned int       m_index;
    std::map<CInputPrimitive, JoystickFeatureID> m_actions;
    JoystickFeatureMap       m_features;
  };
}
