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
#include "guilib/Key.h"
#include "input/ButtonTranslator.h"
#include "windowing/WinEvents.h"
#include "Application.h"

bool CGenericJoystickFeatureHandler::OnButtonPress(JoystickFeatureID id)
{
  unsigned int keyId = GetButtonID(id);
  if (keyId)
  {
    CAction action(CButtonTranslator::GetInstance().GetAction(g_application.GetActiveWindowID(), CKey(keyId, 0)));
    if (action.GetID() > 0)
    {
      if (CButtonTranslator::IsAnalog(action.GetID()))
      {
        SendAnalogButton(keyId, 1.0f);
      }
      else
      {
        SendDigitalButton(keyId);
        // TODO: Start hold timer
      }
    }
  }

  return true;
}

bool CGenericJoystickFeatureHandler::OnButtonMotion(JoystickFeatureID id, float magnitude)
{
  unsigned int keyId = GetButtonID(id);
  if (keyId)
  {
    CAction action(CButtonTranslator::GetInstance().GetAction(g_application.GetActiveWindowID(), CKey(keyId, 0)));
    if (action.GetID() > 0)
    {
      if (CButtonTranslator::IsAnalog(action.GetID()))
      {
        SendAnalogButton(keyId, magnitude);
      }
      else
      {
        bool hasHoldTimer = false;
        if (magnitude >= 0.5f && !hasHoldTimer)
        {
          SendDigitalButton(id);
          // TODO: Start hold timer
        }
        else if (magnitude < 0.5f && hasHoldTimer)
        {
          // TODO: Cancel hold timer
        }
      }
    }
  }

  return true;
}

bool CGenericJoystickFeatureHandler::OnButtonHold(JoystickFeatureID id, unsigned int holdTimeMs)
{
  SendDigitalButton(id, holdTimeMs);

  return true;
}

bool CGenericJoystickFeatureHandler::OnButtonDoublePress(JoystickFeatureID id)
{
  return false; // TODO
}

bool CGenericJoystickFeatureHandler::OnMultiPress(const std::vector<JoystickFeatureID>& ids)
{
  return false; // TODO
}

bool CGenericJoystickFeatureHandler::OnButtonRelease(JoystickFeatureID id)
{
  unsigned int keyId = GetButtonID(id);
  if (keyId)
  {
    CAction action(CButtonTranslator::GetInstance().GetAction(g_application.GetActiveWindowID(), CKey(keyId, 0)));
    if (action.GetID() > 0)
    {
      if (!CButtonTranslator::IsAnalog(action.GetID()))
      {
        // TODO: Cancel hold timer
      }
    }
  }

  return true;
}

bool CGenericJoystickFeatureHandler::OnAnalogStickMotion(JoystickFeatureID id, float x, float y)
{
  return false; // TODO
}

bool CGenericJoystickFeatureHandler::OnAccelerometerMotion(JoystickFeatureID, float x, float y, float z)
{
  return false; // TODO
}

void CGenericJoystickFeatureHandler::SendDigitalButton(unsigned int keyId, unsigned int holdTimeMs /* = 0 */)
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));

  newEvent.joystick.type = XBMC_JOYDIGITAL;
  newEvent.joystick.button = keyId;
  newEvent.joystick.holdtime = holdTimeMs;

  CWinEvents::MessagePush(&newEvent);
}

void CGenericJoystickFeatureHandler::SendAnalogButton(unsigned int keyId, float amount)
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));

  newEvent.joystick.type = XBMC_JOYANALOG;
  newEvent.joystick.button = keyId;
  newEvent.joystick.amount = amount;

  CWinEvents::MessagePush(&newEvent);
}

unsigned int CGenericJoystickFeatureHandler::GetButtonID(JoystickFeatureID id, float x /* = 0.0f */, float y /* = 0.0f */, float z /* = 0.0f */)
{
  switch (id)
  {
  case JOY_ID_BUTTON_A:
    return KEY_BUTTON_A;
  case JOY_ID_BUTTON_B:
    return KEY_BUTTON_B;
  case JOY_ID_BUTTON_X:
    return KEY_BUTTON_X;
  case JOY_ID_BUTTON_Y:
    return KEY_BUTTON_Y;
  case JOY_ID_BUTTON_C:
    return KEY_BUTTON_BLACK;
  case JOY_ID_BUTTON_Z:
    return KEY_BUTTON_WHITE;
  case JOY_ID_BUTTON_START:
    return KEY_BUTTON_START;
  case JOY_ID_BUTTON_SELECT:
    return KEY_BUTTON_BACK;
  case JOY_ID_BUTTON_MODE:
    return KEY_BUTTON_GUIDE;
  case JOY_ID_BUTTON_L:
    return KEY_BUTTON_LEFT_SHOULDER;
  case JOY_ID_BUTTON_R:
    return KEY_BUTTON_RIGHT_SHOULDER;
  case JOY_ID_TRIGGER_L:
    return KEY_BUTTON_LEFT_TRIGGER;
  case JOY_ID_TRIGGER_R:
    return KEY_BUTTON_RIGHT_TRIGGER;
  case JOY_ID_BUTTON_L_STICK:
    return KEY_BUTTON_LEFT_THUMB_BUTTON;
  case JOY_ID_BUTTON_R_STICK:
    return KEY_BUTTON_RIGHT_THUMB_BUTTON;
  case JOY_ID_BUTTON_LEFT:
    return KEY_BUTTON_DPAD_LEFT;
  case JOY_ID_BUTTON_RIGHT:
    return KEY_BUTTON_DPAD_RIGHT;
  case JOY_ID_BUTTON_UP:
    return KEY_BUTTON_DPAD_UP;
  case JOY_ID_BUTTON_DOWN:
    return KEY_BUTTON_DPAD_DOWN;
  case JOY_ID_ANALOG_STICK_L:
    if (y >= x && y > -x)
      return KEY_BUTTON_LEFT_THUMB_STICK_UP;
    else if (y <= x && y < -x)
      return KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
    else if (y >= x && y < -x)
      return KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
    else
      return KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
  case JOY_ID_ANALOG_STICK_R:
    if (y >= x && y > -x)
      return KEY_BUTTON_RIGHT_THUMB_STICK_UP;
    else if (y <= x && y < -x)
      return KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
    else if (y >= x && y < -x)
      return KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
    else
      return KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
  case JOY_ID_ACCELEROMETER:
    return 0; // TODO
  case JOY_ID_BUTTON_UNKNOWN:
  default:
    return 0;
  }
}
