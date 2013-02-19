/*
 *      Copyright (C) 2012 Garrett Brown
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "games/libretro/libretro.h"
#include "guilib/Key.h"
#include "threads/CriticalSection.h"

#include <stdint.h>

// From dinput.h
#define GAMEPAD_BUTTON_COUNT 128 // WINJoystick
#define GAMEPAD_HAT_COUNT    4   // WINJoystick
#define GAMEPAD_AXIS_COUNT   64  // SDLJoystick
#define GAMEPAD_MAX_CONTROLLERS 4

class CRetroPlayerInput
{
public:
  /**
   * An arrow-based device on a gamepad. Legally, no more than two buttons can
   * be pressed, and only if they are adjacent. If no buttons are pressed, the
   * hat is centered.
   */
  struct Hat
  {
    Hat() { up = right = down = left = 0; }
    bool operator==(const Hat &lhs) const { return up == lhs.up && right == lhs.right && down == lhs.down && left == lhs.left; }
    // Iterate through cardinal directions in an ordinal fasion
    unsigned char &operator[](unsigned int i);
    const unsigned char &operator[](unsigned int i) const;
    // Translate this hat into a cardinal direction ("N", "NE", "E", ...) or "CENTERED"
    const char *GetDirection() const;

    // 1 if pressed, 0 if unpressed
    unsigned char up;
    unsigned char right;
    unsigned char down;
    unsigned char left;
  };

  struct Gamepad
  {
    CStdString    name;
    int           id;
    unsigned char buttons[GAMEPAD_BUTTON_COUNT];
    unsigned int  buttonCount;
    Hat           hats[GAMEPAD_HAT_COUNT];
    unsigned int  hatCount;
    float         axes[GAMEPAD_AXIS_COUNT];
    unsigned int  axisCount;
  };

  CRetroPlayerInput();
  ~CRetroPlayerInput() { Finish(); }

  void Begin();
  void Finish();

  /**
   * Called by the game client to query gamepad states.
   * \param port   The player #. Player 1 is port 0.
   * \param device The fundamental device abstraction. This can be changed from
   *               the default by calling CGameClient::SetDevice().
   * \param index  Only used for analog devices (RETRO_DEVICE_ANALOG)
   * \param id     The button ID being queried.
   * \return       Although this returns int16_t, it seems game clients cast
   *               to bool. Therefore, 0 = not pressed, 1 = pressed.
   */
  int16_t GetInput(unsigned port, unsigned device, unsigned index, unsigned id);

  /**
   * Marks a key as pressed. This intercepts keys sent to CApplication::OnKey()
   * before they are translated into actions.
   */
  void ProcessKeyDown(const CKey &key, unsigned controller_id);

  /**
   * Marks a key as released. Because key releases aren't processed by
   * CApplication and aren't translated into actions, these are intercepted
   * at the raw event stage in CApplication::OnEvent().
   */
  void ProcessKeyUp(const CKey &key, unsigned controller_id);

  /**
   * Monitor gamepads for input changes. This is called by g_Joystick.Update()
   * on every frame, once per gamepad device. Currently, gamepad axes are
   * ignored.
   */
  void ProcessGamepad(const Gamepad &gamepad);

private:
  /**
   * Translate an action ID, found in Key.h, to the corresponding RetroPad ID.
   * Returns -1 if the ID is invalid for the device given by m_device
   * (currently, m_device doesn't exist and this value is forced to
   * RETRO_DEVICE_JOYPAD).
   */
  int TranslateActionID(int id) const;

  bool m_bActive; // Unused currently

  // RETRO_DEVICE_ID_JOYPAD_R3 is the last key in libretro.h
  int16_t m_joypadState[GAMEPAD_MAX_CONTROLLERS][ACTION_JOYPAD_CONTROL_END - ACTION_GAME_CONTROL_START + 1];
  Gamepad m_gamepad[GAMEPAD_MAX_CONTROLLERS];

  CCriticalSection m_statesGuard;
};
