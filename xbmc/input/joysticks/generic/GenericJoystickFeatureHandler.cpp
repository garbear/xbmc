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

#include "GenericJoystickFeatureHandler.h"
#include "GenericJoystickActionHandler.h"

#include <algorithm>

#define HOLD_TIMEOUT_MS    500 // TODO
#define REPEAT_TIMEOUT_MS  250 // TODO

#ifndef ABS
#define ABS(x)  ((x) >= 0 ? (x) : (-x))
#endif

#ifndef MAX
#define MAX(x, y)  ((x) >= (y) ? (x) : (y))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))
#endif

CGenericJoystickFeatureHandler::CGenericJoystickFeatureHandler(void) :
  m_actionHandler(new CGenericJoystickActionHandler),
  m_holdTimer(this)
{
}

CGenericJoystickFeatureHandler::~CGenericJoystickFeatureHandler(void)
{
  delete m_actionHandler;
}

bool CGenericJoystickFeatureHandler::OnButtonPress(JoystickFeatureID id)
{
  unsigned int buttonId = m_actionHandler->GetButtonID(id);
  if (buttonId)
  {
    if (m_actionHandler->IsAnalog(buttonId))
      m_actionHandler->OnAnalogAction(buttonId, 1.0f);
    else
      ProcessButtonPress(buttonId);
  }

  return true;
}

bool CGenericJoystickFeatureHandler::OnButtonRelease(JoystickFeatureID id)
{
  unsigned int buttonId = m_actionHandler->GetButtonID(id);
  if (buttonId)
  {
    if (m_actionHandler->IsAnalog(buttonId))
      m_actionHandler->OnAnalogAction(buttonId, 0.0f);
    else
      ProcessButtonRelease(buttonId);
  }

  return true;
}

bool CGenericJoystickFeatureHandler::OnButtonMotion(JoystickFeatureID id, float magnitude)
{
  unsigned int buttonId = m_actionHandler->GetButtonID(id);
  if (buttonId)
  {
    if (m_actionHandler->IsAnalog(buttonId))
    {
      m_actionHandler->OnAnalogAction(buttonId, magnitude);
    }
    else
    {
      std::set<unsigned int>::iterator it = m_pressedButtons.find(buttonId);

      if (magnitude >= 0.5f && it == m_pressedButtons.end())
        ProcessButtonPress(buttonId);
      else if (magnitude < 0.5f && it != m_pressedButtons.end())
        ProcessButtonRelease(buttonId);
    }
  }

  return true;
}

bool CGenericJoystickFeatureHandler::OnAnalogStickMotion(JoystickFeatureID id, float x, float y)
{
  unsigned int buttonId  = m_actionHandler->GetButtonID(id, x, y);

  float magnitude = MAX(ABS(x), ABS(y));

  unsigned int buttonRightId = m_actionHandler->GetButtonID(id,  1.0f,  0.0f);
  unsigned int buttonUpId    = m_actionHandler->GetButtonID(id,  0.0f,  1.0f);
  unsigned int buttonLeftId  = m_actionHandler->GetButtonID(id, -1.0f,  0.0f);
  unsigned int buttonDownId  = m_actionHandler->GetButtonID(id,  0.0f, -1.0f);
  
  unsigned int buttonIds[] = {buttonRightId, buttonUpId, buttonLeftId, buttonDownId};

  for (unsigned int i = 0; i < ARRAY_SIZE(buttonIds); i++)
  {
    if (!buttonIds[i])
      continue;
    
    std::set<unsigned int>::iterator it = m_pressedButtons.find(buttonIds[i]);
    
    if (m_actionHandler->IsAnalog(buttonId))
    {
      if (buttonId == buttonIds[i])
      {
        if (it != m_pressedButtons.end())
          m_pressedButtons.insert(buttonId);

        m_actionHandler->OnAnalogAction(buttonId, magnitude);
      }
      else if (it != m_pressedButtons.end())
      {
        m_pressedButtons.erase(it);
        m_actionHandler->OnAnalogAction(buttonId, 0.0f);
      }
    }
    else
    {
      if (buttonId == buttonIds[i])
      {
        if (magnitude >= 0.5f && it == m_pressedButtons.end())
          ProcessButtonPress(buttonId);
        else if (magnitude < 0.5f && it != m_pressedButtons.end())
          ProcessButtonRelease(buttonId);
      }
      else if (it != m_pressedButtons.end())
      {
        ProcessButtonRelease(buttonId);
      }
    }
  }

  return true;
}

bool CGenericJoystickFeatureHandler::OnAccelerometerMotion(JoystickFeatureID id, float x, float y, float z)
{
  return OnAnalogStickMotion(id, x, y); // TODO
}

void CGenericJoystickFeatureHandler::OnTimeout(void)
{
  if (m_lastButtonPress && m_holdTimer.GetElapsedMilliseconds() >= HOLD_TIMEOUT_MS)
    m_actionHandler->OnDigitalAction(m_lastButtonPress, (unsigned int)m_holdTimer.GetElapsedMilliseconds());
}

void CGenericJoystickFeatureHandler::ProcessButtonPress(unsigned int buttonId)
{
  m_pressedButtons.insert(buttonId);
  m_actionHandler->OnDigitalAction(buttonId);
  StartHoldTimer(buttonId);
}

void CGenericJoystickFeatureHandler::ProcessButtonRelease(unsigned int buttonId)
{
  std::set<unsigned int>::iterator it = m_pressedButtons.find(buttonId);
  if (it != m_pressedButtons.end())
    m_pressedButtons.erase(it);

  if (buttonId == m_lastButtonPress || m_pressedButtons.empty())
    ClearHoldTimer();
}

void CGenericJoystickFeatureHandler::StartHoldTimer(unsigned int buttonId)
{
  ClearHoldTimer();
  m_holdTimer.Start(REPEAT_TIMEOUT_MS, true);
  m_lastButtonPress = buttonId;
}

void CGenericJoystickFeatureHandler::ClearHoldTimer(void)
{
  m_holdTimer.Stop(true);
  m_lastButtonPress = 0;
}
