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

#include "GenericJoystickInputHandler.h"
#include "input/joysticks/InputPrimitive.h"
#include "input/joysticks/IJoystickFeatureHandler.h"
#include "input/joysticks/IJoystickButtonMap.h"

#define ANALOG_DIGITAL_THRESHOLD  0.5f

CGenericJoystickInputHandler::CGenericJoystickInputHandler(IJoystickFeatureHandler *handler, IJoystickButtonMap* buttonMap)
 : m_handler(handler),
   m_buttonMap(buttonMap)
{
}

void CGenericJoystickInputHandler::OnButtonMotion(unsigned int index, bool bPressed)
{
  const char pressed = bPressed ? 1 : 0;

  if (m_buttonStates.size() <= index)
    m_buttonStates.resize(index + 1);

  if (m_buttonStates[index] == pressed)
    return;
  
  char& oldState = m_buttonStates[index];

  CInputPrimitive button(index);
  JoystickFeatureID action = m_buttonMap->GetAction(button);

  if (action)
  {
    if (!oldState && pressed)
      m_handler->OnButtonPress(action);
    else if (oldState && !pressed)
      m_handler->OnButtonRelease(action);
  }

  oldState = pressed;
}

void CGenericJoystickInputHandler::OnHatMotion(unsigned int index, HatDirection direction)
{
  if (m_hatStates.size() <= index)
    m_hatStates.resize(index + 1);

  HatDirection& oldDirection = m_hatStates[index];

  // Check left press/release
  if (!(oldDirection & HatDirectionLeft) && (direction & HatDirectionLeft))
  {
    CInputPrimitive left(index, HatDirectionLeft);
    JoystickFeatureID action = m_buttonMap->GetAction(left);
    if (action)
      m_handler->OnButtonPress(action);
  }
  else if ((oldDirection & HatDirectionLeft) && !(direction & HatDirectionLeft))
  {
    CInputPrimitive left(index, HatDirectionLeft);
    JoystickFeatureID action = m_buttonMap->GetAction(left);
    if (action)
      m_handler->OnButtonRelease(action);
  }

  // Check right press/release
  if (!(oldDirection & HatDirectionRight) && (direction & HatDirectionRight))
  {
    CInputPrimitive right(index, HatDirectionRight);
    JoystickFeatureID action = m_buttonMap->GetAction(right);
    if (action)
      m_handler->OnButtonPress(action);
  }
  else if ((oldDirection & HatDirectionRight) && !(direction & HatDirectionRight))
  {
    CInputPrimitive right(index, HatDirectionRight);
    JoystickFeatureID action = m_buttonMap->GetAction(right);
    if (action)
      m_handler->OnButtonRelease(action);
  }

  // Check up press/release
  if (!(oldDirection & HatDirectionUp) && (direction & HatDirectionUp))
  {
    CInputPrimitive up(index, HatDirectionUp);
    JoystickFeatureID action = m_buttonMap->GetAction(up);
    if (action)
      m_handler->OnButtonPress(action);
  }
  else if ((oldDirection & HatDirectionUp) && !(direction & HatDirectionUp))
  {
    CInputPrimitive up(index, HatDirectionUp);
    JoystickFeatureID action = m_buttonMap->GetAction(up);
    if (action)
      m_handler->OnButtonRelease(action);
  }

  // Check down press/release
  if (!(oldDirection & HatDirectionDown) && (direction & HatDirectionDown))
  {
    CInputPrimitive down(index, HatDirectionDown);
    JoystickFeatureID action = m_buttonMap->GetAction(down);
    if (action)
      m_handler->OnButtonPress(action);
  }
  else if ((oldDirection & HatDirectionDown) && !(direction & HatDirectionDown))
  {
    CInputPrimitive down(index, HatDirectionDown);
    JoystickFeatureID action = m_buttonMap->GetAction(down);
    if (action)
      m_handler->OnButtonRelease(action);
  }

  oldDirection = direction;
}

void CGenericJoystickInputHandler::OnAxisMotion(unsigned int index, float position)
{
  if (m_axisStates.size() <= index)
    m_axisStates.resize(index + 1);

  if (m_axisStates[index] == position)
    return;

  float oldPosition = m_axisStates[index];
  m_axisStates[index] = position;

  CInputPrimitive positiveAxis(index, SemiAxisDirectionPositive);
  CInputPrimitive negativeAxis(index, SemiAxisDirectionNegative);

  JoystickFeatureID positiveAction = m_buttonMap->GetAction(positiveAxis);  
  JoystickFeatureID negativeAction = m_buttonMap->GetAction(negativeAxis);

  if (!positiveAction && !negativeAction)
  {
    // No actions to send to callback
  }
  else if (positiveAction == negativeAction)
  {
    // Feature uses multiple axes, add to OnAxisMotions() batch process
    m_featuresWithMotion.insert(positiveAction);
  }
  else // positiveAction != negativeAction
  {
    // Positive and negative directions are mapped to different actions, so we
    // must be dealing with a button or trigger

    if (positiveAction)
    {
      // If position passes through zero, 0.0f is sent exactly once until the
      // position becomes positive again
      if (position > 0)
        m_handler->OnButtonMotion(positiveAction, position);
      else if (oldPosition > 0)
        m_handler->OnButtonMotion(positiveAction, 0.0f);
    }

    if (negativeAction)
    {
      // If position passes through zero, 0.0f is sent exactly once until the
      // position becomes negative again
      if (position < 0)
        m_handler->OnButtonMotion(negativeAction, -1.0f * position); // magnitude is >= 0
      else if (oldPosition < 0)
        m_handler->OnButtonMotion(negativeAction, 0.0f);
    }
  }
}

void CGenericJoystickInputHandler::ProcessAxisMotions()
{
  std::set<JoystickFeatureID> featuresToProcess;
  featuresToProcess.swap(m_featuresWithMotion);

  for (std::set<JoystickFeatureID>::const_iterator it = featuresToProcess.begin(); it != featuresToProcess.end(); ++it)
  {
    const JoystickFeatureID action = *it;
    switch (action)
    {
    case JOY_ID_ANALOG_STICK_L:
    case JOY_ID_ANALOG_STICK_R:
    {
      int  horizIndex;
      bool horizInverted;
      int  vertIndex;
      bool vertInverted;

      if (m_buttonMap->GetAnalogStick(action,
                                      horizIndex, horizInverted,
                                      vertIndex,  vertInverted))
      {
        const float horizPos = GetAxisState(horizIndex) * (horizInverted ? -1.0f : 1.0f);
        const float vertPos  = GetAxisState(vertIndex)  * (vertInverted  ? -1.0f : 1.0f);
        m_handler->OnAnalogStickMotion(action, horizPos, vertPos);
      }
      break;
    }

    case JOY_ID_ACCELEROMETER:
    {
      int  xIndex;
      bool xInverted;
      int  yIndex;
      bool yInverted;
      int  zIndex;
      bool zInverted;

      if (m_buttonMap->GetAccelerometer(action,
                                        xIndex, xInverted,
                                        yIndex, yInverted,
                                        zIndex, zInverted))
      {
        const float xPos = GetAxisState(xIndex) * (xInverted ? -1.0f : 1.0f);
        const float yPos = GetAxisState(yIndex) * (yInverted ? -1.0f : 1.0f);
        const float zPos = GetAxisState(zIndex) * (zInverted ? -1.0f : 1.0f);
        m_handler->OnAccelerometerMotion(action, xPos, yPos, zPos);
      }
      break;
    }

    default:
      break;
    }
  }
}

float CGenericJoystickInputHandler::GetAxisState(int axisIndex) const
{
  return (0 <= axisIndex && axisIndex < (int)m_axisStates.size()) ? m_axisStates[axisIndex] : 0;
}
