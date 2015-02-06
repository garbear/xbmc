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

#include "input/joysticks/IJoystickFeatureHandler.h"
#include "threads/Timer.h"

#include <set>

class IJoystickActionHandler;

/*!
 * \ingroup joysticks_generic
 * \brief Generic implementation of IJoystickFeatureHandler to translate
 *        joystick features into joystick.xml buttons
 *
 * \sa IJoystickFeatureHandler
 */
class CGenericJoystickFeatureHandler : public IJoystickFeatureHandler, public ITimerCallback
{
public:
  CGenericJoystickFeatureHandler(void);

  virtual ~CGenericJoystickFeatureHandler(void);

  // implementation of IJoystickFeatureHandler
  virtual bool OnButtonPress(JoystickFeatureID id);
  virtual bool OnButtonRelease(JoystickFeatureID id);
  virtual bool OnButtonMotion(JoystickFeatureID id, float magnitude);
  virtual bool OnAnalogStickMotion(JoystickFeatureID id, float x, float y);
  virtual bool OnAccelerometerMotion(JoystickFeatureID id, float x, float y, float z);

  // implementation of ITimerCallback
  virtual void OnTimeout(void);

private:
  void ProcessButtonPress(unsigned int buttonId);
  void ProcessButtonRelease(unsigned int buttonId);

  void StartHoldTimer(unsigned int buttonId);
  void ClearHoldTimer(void);

  IJoystickActionHandler* const m_actionHandler;
  CTimer                        m_holdTimer;
  unsigned int                  m_lastButtonPress;
  std::set<unsigned int>        m_pressedButtons;
};
