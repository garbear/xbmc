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

#include "input/joysticks/IJoystickFeatureHandler.h"

/*!
 * \ingroup joysticks_generic
 * \brief Generic implementation of IJoystickFeatureHandler to translate
 *        joystick actions into XBMC specific and mappable actions.
 *
 * \sa IJoystickFeatureHandler
 */
class CGenericJoystickFeatureHandler : public IJoystickFeatureHandler
{
public:
  CGenericJoystickFeatureHandler() { }

  virtual ~CGenericJoystickFeatureHandler() { }

  // implementation of IJoystickFeatureHandler
  virtual bool OnButtonPress(JoystickFeatureID id);
  virtual bool OnButtonMotion(JoystickFeatureID id, float magnitude);
  virtual bool OnButtonHold(JoystickFeatureID id, unsigned int holdTimeMs);
  virtual bool OnButtonDoublePress(JoystickFeatureID id);
  virtual bool OnMultiPress(const std::vector<JoystickFeatureID>& ids);
  virtual bool OnButtonRelease(JoystickFeatureID id);
  virtual bool OnAnalogStickMotion(JoystickFeatureID id, float x, float y);
  virtual bool OnAccelerometerMotion(JoystickFeatureID id, float x, float y, float z);

private:
  static void SendDigitalButton(unsigned int keyId, unsigned int holdTimeMs = 0);
  static void SendAnalogButton(unsigned int keyId, float amount);

  static unsigned int GetButtonID(JoystickFeatureID id, float x = 0.0f, float y = 0.0f, float z = 0.0f);
};
