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

#include "system.h" // for HAS_JOYSTICK
#if defined(HAS_JOYSTICK)

#include "JoystickManager.h"
#include "peripherals/devices/PeripheralImon.h"
#include "settings/Setting.h"
#include "utils/StdString.h"

// Include joystick APIs
#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystickXInput.h"
#include "input/windows/WINJoystickDX.h"
#endif

#if defined(HAS_LINUX_JOYSTICK)
#include "input/linux/LinuxJoystick.h"
#elif defined(HAS_SDL_JOYSTICK)
#include "input/linux/LinuxJoystickSDL.h"
#endif

using namespace JOYSTICK;
using namespace PERIPHERALS;
using namespace std;

CJoystickManager &CJoystickManager::Get()
{
  static CJoystickManager joystickManager;
  return joystickManager;
}

void CJoystickManager::Initialize()
{
  if (!IsEnabled())
    return;
  
  // Initialize joystick APIs
#if defined(TARGET_WINDOWS)
  CJoystickXInput::Initialize(m_joysticks);
  CJoystickDX::Initialize();
#endif

#if defined(HAS_LINUX_JOYSTICK)
  CLinuxJoystick::Initialize(m_joysticks);
#elif defined(HAS_SDL_JOYSTICK)
  CLinuxJoystickSDL::Initialize(m_joysticks);
#endif

  // Truncate array
  while (m_joysticks.size() > GAMEPAD_MAX_CONTROLLERS)
    m_joysticks.pop_back();

  for (JoystickArray::iterator it = m_joysticks.begin(); it != m_joysticks.end(); it++)
  (*it)->ResetState();
}

void CJoystickManager::DeInitialize()
{
  // De-initialize joystick APIs
#if defined(TARGET_WINDOWS)
  CJoystickXInput::DeInitialize(m_joysticks);
  CJoystickDX::DeInitialize(m_joysticks);
#endif

#if defined(HAS_LINUX_JOYSTICK)
  CLinuxJoystick::DeInitialize(m_joysticks);
#elif defined(HAS_SDL_JOYSTICK)
  CLinuxJoystickSDL::DeInitialize(m_joysticks);
#endif

  for (JoystickArray::iterator it = m_joysticks.begin(); it != m_joysticks.end(); it++)
    (*it)->ResetState();

}

void CJoystickManager::Update()
{
  if (!IsEnabled())
    return;

  for (JoystickArray::iterator it = m_joysticks.begin(); it != m_joysticks.end(); it++) {
    (*it)->Update();
    (*it)->UpdateState();
  }

}


void CJoystickManager::SetEnabled(bool enabled /* = true */)
{
  if (enabled && !m_bEnabled)
  {
    m_bEnabled = true;
    Initialize();
  }
  else if (!enabled && m_bEnabled)
  {
    DeInitialize();
    m_bEnabled = false;
  }
}

void CJoystickManager::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "input.enablejoystick")
  {
    SetEnabled(((CSettingBool*)setting)->GetValue() && CPeripheralImon::GetCountOfImonsConflictWithDInput() == 0);
  }
}

#endif // defined(HAS_JOYSTICK)
