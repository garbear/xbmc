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

#include "InputPrimitive.h"

#include <stdint.h>
#include <string>

/*!
 * \ingroup joysticks
 *
 * \brief Button map interface to translate between raw button/hat/axis elements
 *        and physical joystick features.
 */
class IJoystickButtonMap
{
public:
  virtual ~IJoystickButtonMap() { }

  /*!
   * \brief Load the button map into memory
   *
   * \return true if button map is ready to start translating buttons, false otherwise
   */
  virtual bool Load(void) = 0;

  /*!
   * \brief Get the action ID associated with in input primitive
   *
   * \param button The input primitive (a button, hat direction or semi-axis)
   *
   * \return The ID, or UNKNOWN if input primitive isn't associated with an
   *         action ID
   */
  virtual JoystickFeatureID GetAction(const CInputPrimitive& button) = 0;

  /*!
   * \brief Get the raw button, raw hat direction or raw semi-axis associated
   *        with the given button/trigger ID
   *
   * \param id      The action ID. ID must correspond to a single button
   *                primitive, so no analog sticks or accelerometers.
   * \param button  The resolved input primitive
   *
   * \return true if the ID resolved to an input primitive, false if the ID was
   *         invalid or resolved to an analog stick/accelerometer
   */
  virtual bool GetInputPrimitive(JoystickFeatureID id, CInputPrimitive& button) = 0;

  /*!
   * \brief Get the raw axis indices and polarity for the given analog stick ID
   *
   * \param horizIndex     The index of the axis corresponding to the analog
   *                       stick's horizontal motion, or -1 if unknown
   * \param horizInverted  false if right is positive, true if right is negative
   * \param vertIndex      The index of the axis corresponding to the analog
   *                       stick's vertical motion, or -1 if unknown
   * \param vertInverted   false if up is positive, true if up is negative
   *
   * \return true if the ID resolved to at least one axis (remaining axis may be -1)
   */
  virtual bool GetAnalogStick(JoystickFeatureID id, int& horizIndex, bool& horizInverted,
                                                    int& vertIndex,  bool& vertInverted) = 0;

  /*!
   * \brief Get the raw axis indices and polarity for the given accelerometer ID
   *
   * \param xIndex     The index of the axis corresponding to the accelerometer's
   *                   X-axis, or -1 if unknown
   * \param xInverted  false if positive X is positive, true if positive X is negative
   * \param yIndex     The index of the axis corresponding to the accelerometer's
   *                   Y-axis, or -1 if unknown
   * \param yInverted  false if positive Y is positive, true if positive Y is negative
   * \param zIndex     The index of the axis corresponding to the accelerometer's
   *                   Z-axis, or -1 if unknown
   * \param zInverted  false if positive X is positive, true if positive Z is negative
   *
   * \return true if the ID resolved to at least one axis (remaining axes may be -1)
   */
  virtual bool GetAccelerometer(JoystickFeatureID id, int& xIndex, bool& xInverted,
                                                      int& yIndex, bool& yInverted,
                                                      int& zIndex, bool& zInverted) = 0;
};
