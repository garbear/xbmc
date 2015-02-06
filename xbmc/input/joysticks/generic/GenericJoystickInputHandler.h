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

#include "input/joysticks/IJoystickInputHandler.h"

#include <set>
#include <vector>

class IJoystickFeatureHandler;
class IJoystickButtonMap;

/*!
 * \brief Generic implementation of IJoystickInputHandler to translate raw
 *        actions into physical joystick features using the provided button map
 */
class CGenericJoystickInputHandler : public IJoystickInputHandler
{
public:
  CGenericJoystickInputHandler(IJoystickFeatureHandler *handler, IJoystickButtonMap *buttonMap);

  virtual ~CGenericJoystickInputHandler() { }

  // implementation of IJoystickInputHandler
  virtual void OnButtonMotion(unsigned int index, bool bPressed);
  virtual void OnHatMotion(unsigned int index, HatDirection direction);
  virtual void OnAxisMotion(unsigned int index, float position);
  virtual void ProcessAxisMotions();

private:
  float GetAxisState(int axisIndex) const;

  IJoystickFeatureHandler     *m_handler;
  IJoystickButtonMap          *m_buttonMap;
  std::vector<char>           m_buttonStates; // std::vector is specialized for <bool>
  std::vector<HatDirection>   m_hatStates;
  std::vector<float>          m_axisStates;
  std::set<JoystickFeatureID> m_featuresWithMotion;
};
