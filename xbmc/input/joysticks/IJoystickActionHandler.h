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

#include "JoystickTypes.h"

/*!
 * \ingroup joysticks
 * \brief Interface for handling events for button IDs used in joystick.xml
 */
class IJoystickActionHandler
{
public:
  virtual ~IJoystickActionHandler(void) { }

  /*!
   * \brief Get the button ID, as defined in guilib/Key.h, for the specified
   *        joystick feature/direction
   *
   * To obtain the ID of analog stick directions (e.g. the "rightthumbstickup"
   * button), a direction vector can be provided in the optional parameters.
   * Ties are resolved by assuming the clockwise direction; up right (x=0.5, y=0.5)
   * will resolve to right.
   *
   * \param id        The joystick feature ID
   * \param x         The x component of the direction being queried
   * \param y         The y component of the direction being queried
   * \param z         The z component of the direction being queried
   *
   * \return True if the event was handled otherwise false
   */
  virtual unsigned int GetButtonID(JoystickFeatureID id, float x = 0.0f, float y = 0.0f, float z = 0.0f) = 0;

  /*!
   * \brief Determine if the button ID is mapped to a digital or analog action
   *
   * \param buttonId       The button ID
   *
   * \return True if the button ID is mapped to a digital action, otherwise false
   *
   * \sa CButtonTranslator::IsAnalog()
   */
  virtual bool IsAnalog(unsigned int buttonId) = 0;

  /*!
   * \brief A button mapped to a digital action has been pressed
   *
   * \param buttonId        The button ID
   * \param holdTimeMs      For repeat events, the duration the button has been held
   */
  virtual void OnDigitalAction(unsigned int buttonId, unsigned int holdTimeMs = 0) = 0;

  /*!
   * \brief A button ID mapped to an analog action has experienced a motion
   *
   * \param buttonId        The button ID
   * \param amount          The amount of motion in the closed interval [0.0, 1.0]
   */
  virtual void OnAnalogAction(unsigned int buttonId, float amount) = 0;
};
