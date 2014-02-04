/*
 *      Copyright (C) 2007-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "IJoystick.h"
#include "settings/ISettingCallback.h"

#include <string>

namespace JOYSTICK
{

/**
 * Class to manage all connected joysticks.
 */
class CJoystickManager : public ISettingCallback
{
private:
  CJoystickManager() : m_bEnabled(false) { }
  virtual ~CJoystickManager() { DeInitialize(); }

public:
  static CJoystickManager &Get();

  void SetEnabled(bool enabled = true);
  bool IsEnabled() const { return m_bEnabled; }
  void Update();
  unsigned int Count() const { return m_joysticks.size(); }
  void Reinitialize() { Initialize(); }
  void Reset() { 
    //Action tracker removed, what does this do now? 
  }

  // Inherited from ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);

private:
  void Initialize();
  void DeInitialize();

  JoystickArray m_joysticks;
  bool          m_bEnabled;

};

} // namespace INPUT
