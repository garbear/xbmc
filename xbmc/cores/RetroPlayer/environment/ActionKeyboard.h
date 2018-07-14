/*
 *      Copyright (C) 2018 Team Kodi
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

#include "input/keyboard/KeyboardTypes.h"

namespace KODI
{
namespace RETRO
{
  using KeyboardModifier = KEYBOARD::Modifier;
  using KeyboardSymbol = KEYBOARD::KeySymbol;

  /*!
   * \brief The properties of a pressed key
   */
  class CKeyPress
  {
  public:
    CKeyPress();
    ~CKeyPress();

    /*!
     * \brief The hardware-independent symbol identifying the pressed key
     */
    KeyboardSymbol m_symbol = XBMCK_UNKNOWN;

    /*!
     * \brief Unicode value
     *
     * If the keypress generates a printing character, the unicode value
     * contains the character generated. If the key is a non-printing
     * character, e.g. a function or arrow key, the unicode value is zero.
     */
    uint32_t m_unicode = 0;

    /*!
     * \brief The modifiers held for this key press
     */
    KeyboardModifier m_modifiers = static_cast<KeyboardModifier>(0x0);
  };
}
}
