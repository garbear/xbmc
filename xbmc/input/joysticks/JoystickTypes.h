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

/*!
 * \ingroup joysticks
 * \brief Feature IDs for physical elements on a joystick
 */
enum JoystickFeatureID
{
  JOY_ID_BUTTON_UNKNOWN = 0,
  JOY_ID_BUTTON_A,
  JOY_ID_BUTTON_B,
  JOY_ID_BUTTON_X,
  JOY_ID_BUTTON_Y,
  JOY_ID_BUTTON_C,
  JOY_ID_BUTTON_Z,
  JOY_ID_BUTTON_START,
  JOY_ID_BUTTON_SELECT,
  JOY_ID_BUTTON_MODE,
  JOY_ID_BUTTON_L,
  JOY_ID_BUTTON_R,
  JOY_ID_TRIGGER_L,
  JOY_ID_TRIGGER_R,
  JOY_ID_BUTTON_L_STICK,
  JOY_ID_BUTTON_R_STICK,
  JOY_ID_BUTTON_LEFT,
  JOY_ID_BUTTON_RIGHT,
  JOY_ID_BUTTON_UP,
  JOY_ID_BUTTON_DOWN,
  JOY_ID_ANALOG_STICK_L,
  JOY_ID_ANALOG_STICK_R,
  JOY_ID_ACCELEROMETER,
};

/*!
 * \ingroup joysticks
 * \brief States in which a hat (directional pad) can be
 */
enum HatDirection
{
  HatDirectionNone      = 0x0,    /*!< @brief no directions are pressed */
  HatDirectionLeft      = 0x1,    /*!< @brief only left is pressed */
  HatDirectionRight     = 0x2,    /*!< @brief only right is pressed */
  HatDirectionUp        = 0x4,    /*!< @brief only up is pressed */
  HatDirectionDown      = 0x8,    /*!< @brief only down is pressed */
  HatDirectionLeftUp    = HatDirectionLeft  | HatDirectionUp,
  HatDirectionLeftDown  = HatDirectionLeft  | HatDirectionDown,
  HatDirectionRightUp   = HatDirectionRight | HatDirectionUp,
  HatDirectionRightDown = HatDirectionRight | HatDirectionDown,
};

/*!
 * \ingroup joysticks
 * \brief Directions a semiaxis can point
 */
enum SemiAxisDirection
{
  SemiAxisDirectionNegative = -1,  // semiaxis lies in the interval [-1.0, 0.0]
  SemiAxisDirectionUnknown  =  0,  // semiaxis is unknown or invalid
  SemiAxisDirectionPositive =  1,  // semiaxis lies in the interval [0.0, 1.0]
};
