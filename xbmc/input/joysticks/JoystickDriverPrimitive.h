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

#include <stdint.h>

enum DriverPrimitiveType
{
  DriverPrimitiveTypeUnknown = 0,
  DriverPrimitiveTypeButton,
  DriverPrimitiveTypeHatDirection,
  DriverPrimitiveTypeSemiAxis,
};

/*!
 * A driver input primitive is a basic element that emits input events. This can
 * be a button, one of the four direction arrows on a dpad, or the positive or
 * negative half of an axis.
 */
class CJoystickDriverPrimitive
{
public:
  /*!
   * \brief Construct an invalid input primitive
   */
  CJoystickDriverPrimitive(void);

  /*!
   * \brief Construct an input primitive representing a button press
   */
  CJoystickDriverPrimitive(unsigned int buttonIndex);

  /*!
   * \brief Construct an input primitive representing one of the four direction
   *        arrows on a dpad
   */
  CJoystickDriverPrimitive(unsigned int hatIndex, HatDirection direction);

  /*!
   * \brief Construct an input primitive representing the positive or negative
   *        half of an axis
   */
  CJoystickDriverPrimitive(unsigned int axisIndex, SemiAxisDirection direction);

  bool operator==(const CJoystickDriverPrimitive& rhs) const;
  bool operator<(const CJoystickDriverPrimitive& rhs) const;

  bool operator!=(const CJoystickDriverPrimitive& rhs) const { return !operator==(rhs); }
  bool operator>(const CJoystickDriverPrimitive& rhs) const  { return !(operator<(rhs) || operator==(rhs)); }
  bool operator<=(const CJoystickDriverPrimitive& rhs) const { return   operator<(rhs) || operator==(rhs); }
  bool operator>=(const CJoystickDriverPrimitive& rhs) const { return  !operator<(rhs); }

  DriverPrimitiveType Type(void) const        { return m_type; }
  unsigned int        Index(void) const       { return m_index; }
  HatDirection        HatDir(void) const      { return m_hatDirection; }
  SemiAxisDirection   SemiAxisDir(void) const { return m_semiAxisDirection; }

  /*!
   * \brief An input primitive is valid if has a known type and:
   *        - for hats, is a cardinal direction
   *        - for semi-axes, is a positive or negative direction
   */
  bool IsValid(void) const;

private:
  DriverPrimitiveType m_type;
  unsigned int        m_index;
  HatDirection        m_hatDirection;
  SemiAxisDirection   m_semiAxisDirection;
};
