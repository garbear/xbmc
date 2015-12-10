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

#include "GenericKeyboardJoystick.h"
#include "GenericKeyboardJoystickHandling.h"
#include "input/joysticks/IDriverHandler.h"
#include "input/Key.h"
#include "threads/SingleLock.h"

using namespace KEYBOARD;

void CGenericKeyboardJoystick::RegisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler, bool bPromiscuous)
{
  CSingleLock lock(m_mutex);

  if (m_keyboardHandlers.find(handler) == m_keyboardHandlers.end())
    m_keyboardHandlers[handler] = KeyboardHandle{new CGenericKeyboardJoystickHandling(handler), bPromiscuous};
}

void CGenericKeyboardJoystick::UnregisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler)
{
  CSingleLock lock(m_mutex);

  KeyboardHandlers::iterator it = m_keyboardHandlers.find(handler);
  if (it != m_keyboardHandlers.end())
  {
    delete it->second.handler;
    m_keyboardHandlers.erase(it);
  }
}

bool CGenericKeyboardJoystick::OnKeyPress(const CKey& key)
{
  CSingleLock lock(m_mutex);

  bool bHandled = false;

  // Process promiscuous handlers
  for (KeyboardHandlers::iterator it = m_keyboardHandlers.begin(); it != m_keyboardHandlers.end(); ++it)
  {
    if (it->second.bPromiscuous)
      it->second.handler->OnKeyPress(key);
  }

  // Process handlers until one is handled
  for (KeyboardHandlers::iterator it = m_keyboardHandlers.begin(); it != m_keyboardHandlers.end(); ++it)
  {
    if (!it->second.bPromiscuous)
    {
      bHandled = it->second.handler->OnKeyPress(key);
      if (bHandled)
        break;
    }
  }

  return bHandled;
}

void CGenericKeyboardJoystick::OnKeyRelease(const CKey& key)
{
  CSingleLock lock(m_mutex);

  for (KeyboardHandlers::iterator it = m_keyboardHandlers.begin(); it != m_keyboardHandlers.end(); ++it)
    it->second.handler->OnKeyRelease(key);
}
