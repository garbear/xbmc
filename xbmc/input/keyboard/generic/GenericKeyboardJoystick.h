/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "input/keyboard/IKeyboardHandler.h"
#include "threads/CriticalSection.h"

#include <map>

namespace JOYSTICK
{
  class IDriverHandler;
}

namespace KEYBOARD
{
  class CGenericKeyboardJoystick : public IKeyboardHandler
  {
  public:
    CGenericKeyboardJoystick(void) { }

    virtual ~CGenericKeyboardJoystick(void) { }

    void RegisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler, bool bPromiscuous);
    void UnregisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler);

    // implementation of IKeyboardHandler
    virtual bool OnKeyPress(const CKey& key) override;
    virtual void OnKeyRelease(const CKey& key) override;

  private:
    struct KeyboardHandle
    {
      IKeyboardHandler* handler;
      bool              bPromiscuous;
    };

    typedef std::map<JOYSTICK::IDriverHandler*, KeyboardHandle> KeyboardHandlers;

    KeyboardHandlers m_keyboardHandlers;
    CCriticalSection m_mutex;
  };
}
