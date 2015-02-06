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
 * \brief Interface defining methods to handle raw joystick events (buttons,
 *        hats, axes)
 */
class IJoystickInputHandler
{
public:
  virtual ~IJoystickInputHandler() { }

  /*!
   * \brief Handle button motion
   *
   * \param index     The index of the raw button
   * \param bPressed  true for press motion, false for release motion
   */
  virtual void OnButtonMotion(unsigned int index, bool bPressed) = 0;

  /*!
   * \brief Handle hat motion
   *
   * \param index      The index of the raw hat
   * \param direction  The direction the hat is now being pressed
   */
  virtual void OnHatMotion(unsigned int index, HatDirection direction) = 0;

  /*!
   * \brief Handle axis motion
   *
   * \param index     The index of the raw axis
   * \param position  The position of the axis in the closed interval [-1.0, 1.0]
   */
  virtual void OnAxisMotion(unsigned int index, float position) = 0;
  
  virtual void ProcessAxisMotions(void) = 0;
};
