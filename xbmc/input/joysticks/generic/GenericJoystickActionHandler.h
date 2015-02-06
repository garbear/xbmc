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

#include "input/joysticks/IJoystickActionHandler.h"

/*!
 * \ingroup joysticks_generic
 * \brief Generic implementation of IJoystickFeatureHandler to translate
 *        joystick actions into XBMC specific and mappable actions.
 *
 * \sa IJoystickFeatureHandler
 */
class CGenericJoystickActionHandler : public IJoystickActionHandler
{
public:
  CGenericJoystickActionHandler() { }

  virtual ~CGenericJoystickActionHandler() { }
  
  // Ties (when abs(x) == abs(y)) go clockwise
  virtual unsigned int GetButtonID(JoystickFeatureID id, float x = 0.0f, float y = 0.0f, float z = 0.0f);

  virtual bool IsAnalog(unsigned int buttonId);

  virtual void OnDigitalAction(unsigned int buttonId, unsigned int holdTimeMs = 0);

  virtual void OnAnalogAction(unsigned int buttonId, float amount);
};
