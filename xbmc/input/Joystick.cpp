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

#include "Joystick.h"
#include "settings/AdvancedSettings.h"
#include "Application.h"
#include "ButtonTranslator.h"
#include "guilib/Key.h"
#include "MouseStat.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"

#include <string.h>

#define ACTION_FIRST_DELAY      500 // ms
#define ACTION_REPEAT_DELAY     100 // ms
#define AXIS_DIGITAL_DEADZONE   0.5f // Axis must be pushed past this for digital action repeats

#ifndef ABS
#define ABS(X) ((X) >= 0 ? (X) : (-(X)))
#endif

using namespace JOYSTICK;

void ActionTracker::Reset()
{
  actionID = 0;
  name.clear();
  timeout.SetInfinite();
}

void ActionTracker::Track(const CAction &action)
{
  if (actionID != action.GetID())
  {
    // A new button was pressed, send the action and start tracking it
    actionID = action.GetID();
    name = action.GetName();
    timeout.Set(ACTION_FIRST_DELAY);
  }
  else if (timeout.IsTimePast())
  {
    // Same button was pressed, send the action if the repeat delay has elapsed
    timeout.Set(ACTION_REPEAT_DELAY);
  }
}

void Hat::Center()
{
  up = right = down = left = false;
}

bool Hat::operator==(const Hat &rhs) const
{
  return up == rhs.up && right == rhs.right && down == rhs.down && left == rhs.left;
}

bool &Hat::operator[](unsigned int i)
{
  switch (i)
  {
  case 0:  return up;
  case 1:  return right;
  case 2:  return down;
  case 3:
  default: return left;
  }
}

#define HAT_MAKE_DIRECTION(n, e, s, w) ((n ? 1 : 0) << 3 | (e ? 1 : 0) << 2 | (s ? 1 : 0) << 1 | (w ? 1 : 0))
const char *Hat::GetDirection() const
{
  switch (HAT_MAKE_DIRECTION(up, right, down, left))
  {
  case HAT_MAKE_DIRECTION(1, 0, 0, 0): return "N";
  case HAT_MAKE_DIRECTION(1, 1, 0, 0): return "NE";
  case HAT_MAKE_DIRECTION(0, 1, 0, 0): return "E";
  case HAT_MAKE_DIRECTION(0, 1, 1, 0): return "SE";
  case HAT_MAKE_DIRECTION(0, 0, 1, 0): return "S";
  case HAT_MAKE_DIRECTION(0, 0, 1, 1): return "SW";
  case HAT_MAKE_DIRECTION(0, 0, 0, 1): return "W";
  case HAT_MAKE_DIRECTION(1, 0, 0, 1): return "NW";
  default:                             return "centered";
  }
}

void JoystickState::ResetState(unsigned int buttonCount /* = GAMEPAD_BUTTON_COUNT */,
                          unsigned int hatCount /* = GAMEPAD_HAT_COUNT */,
                          unsigned int axisCount /* = GAMEPAD_AXIS_COUNT */)
{
  buttons.clear();
  hats.clear();
  axes.clear();

  buttons.resize(buttonCount);
  hats.resize(hatCount);
  axes.resize(axisCount);
}

void JoystickState::SetAxis(unsigned int axis, long value, long maxAxisAmount)
{
  if (axis >= axes.size())
    return;
  if (value > maxAxisAmount)
    value = maxAxisAmount;
  else if (value < -maxAxisAmount)
    value = -maxAxisAmount;

  long deadzoneRange = (long)(g_advancedSettings.m_controllerDeadzone * maxAxisAmount);

  if (value > deadzoneRange)
    axes[axis] = (float)(value - deadzoneRange) / (float)(maxAxisAmount - deadzoneRange);
  else if (value < -deadzoneRange)
    axes[axis] = (float)(value + deadzoneRange) / (float)(maxAxisAmount - deadzoneRange);
  else
    axes[axis] = 0.0f;
}


/* Modify the current state and create the actions. */
JoystickState& JoystickState::operator-=(const JoystickState& rhs) {
        ProcessButtonPresses(rhs);
        ProcessHatPresses(rhs);
        ProcessAxisMotion(rhs);
        
        // If tracking an action and the time has elapsed, execute the action now
  if (m_actionTracker.actionID && m_actionTracker.timeout.IsTimePast())
  {
    CAction action(m_actionTracker.actionID, 1.0f, 0.0f, m_actionTracker.name);
    g_application.ExecuteInputAction(action);
    m_actionTracker.Track(action); // Update the timer
  }

  // Reset the wakeup check, so that the check will be performed for the next button press also
  ResetWakeup();

        return *this;
}

// Subtract this instance's value with the other, and return the new instance 
// with the result
const JoystickState JoystickState::operator-(const JoystickState &other) const {
        return JoystickState(*this) -= other;
}

void JoystickState::ProcessButtonPresses(const JoystickState& newState)
{
  for (unsigned int i = 0; i < newState.buttons.size(); i++)
  {
    if (buttons[i] == newState.buttons[i])
      continue;
    buttons[i] = newState.buttons[i];

    CLog::Log(LOGDEBUG, "Joystick %d button %d %s", id, i + 1, newState.buttons[i] ? "pressed" : "unpressed");

    // Check to see if an action is registered for the button first
    int        actionID;
    CStdString actionName;
    bool       fullrange;
    // Button ID is i + 1
    if (!CButtonTranslator::GetInstance().TranslateJoystickString(g_application.GetActiveWindowID(),
                name.c_str(), i + 1, JACTIVE_BUTTON, actionID, actionName, fullrange))
    {
      CLog::Log(LOGDEBUG, "-> Joystick %d button %d no registered action", id, i + 1);
      continue;
    }
    g_Mouse.SetActive(false);

    // Ignore all button presses during this ProcessStateChanges() if we woke
    // up the screensaver (but always send joypad unpresses)
    if (!Wakeup() && newState.buttons[i])
    {
      CAction action(actionID, 1.0f, 0.0f, actionName);
      g_application.ExecuteInputAction(action);
      // Track the button press for deferred repeated execution
      m_actionTracker.Track(action);
    }
    else if (!newState.buttons[i])
    {
      m_actionTracker.Reset(); // If a button was released, reset the tracker
    }
  }
}

void JoystickState::ProcessHatPresses(const JoystickState& newState)
{
  for (unsigned int i = 0; i < newState.hats.size(); i++)
  {
    Hat &oldHat = hats[i];
    const Hat &newHat = newState.hats[i];
    if (oldHat == newHat)
      continue;

    CLog::Log(LOGDEBUG, "Joystick %d hat %d new direction: %s", id, i + 1, newHat.GetDirection());

    // Up, right, down, left
    for (unsigned int j = 0; j < 4; j++)
    {
      if (oldHat[j] == newHat[j])
        continue;
      oldHat[j] = newHat[j];

      int        actionID;
      CStdString actionName;
      bool       fullrange;
      // Up is (1 << 0), right (1 << 1), down (1 << 2), left (1 << 3). Hat ID is i + 1
      int buttonID = (1 << j) << 16 | (i + 1);
      if (!buttonID || !CButtonTranslator::GetInstance().TranslateJoystickString(g_application.GetActiveWindowID(),
        newState.name.c_str(), buttonID, JACTIVE_HAT, actionID, actionName, fullrange))
      {
        static const char *dir[] = {"UP", "RIGHT", "DOWN", "LEFT"};
        CLog::Log(LOGDEBUG, "-> Joystick %d hat %d direction %s no registered action", id, i + 1, dir[j]);
        continue;
      }
      g_Mouse.SetActive(false);

      // Ignore all button presses during this ProcessStateChanges() if we woke
      // up the screensaver (but always send joypad unpresses)
      if (!Wakeup() && newHat[j])
      {
        CAction action(actionID, 1.0f, 0.0f, actionName);
        g_application.ExecuteInputAction(action);
        // Track the hat press for deferred repeated execution
        m_actionTracker.Track(action);
      }
      else if (!newHat[j])
      {
        // If a hat was released, reset the tracker
        m_actionTracker.Reset();
      }
    }
  }
}

void JoystickState::ProcessAxisMotion(const JoystickState& newState)
{
  for (unsigned int i = 0; i < newState.axes.size(); i++)
  {
    // Absolute magnitude
    float absAxis = ABS(newState.axes[i]);

    // Only send one "centered" message
    if (absAxis < 0.01f)
    {
      if (ABS(axes[i]) < 0.01f)
      {
        // The values might not have been exactly equal, so make them
        axes[i] = newState.axes[i];
        continue;
      }
      CLog::Log(LOGDEBUG, "Joystick %d axis %d centered", id, i + 1);
    }
    // Note: don't overwrite oldState until we know whether the action is analog or digital

    int        actionID;
    CStdString actionName;
    bool       fullrange;
    // Axis ID is i + 1, and negative if newState.axes[i] < 0
    if (!CButtonTranslator::GetInstance().TranslateJoystickString(g_application.GetActiveWindowID(),
      newState.name.c_str(), newState.axes[i] >= 0.0f ? (i + 1) : -(int)(i + 1), JACTIVE_AXIS, actionID,
      actionName, fullrange))
    {
      continue;
    }
    g_Mouse.SetActive(false);

    // Use newState.axes[i] as the second about so subscribers can recover the original value
    CAction action(actionID, fullrange ? (newState.axes[i] + 1.0f) / 2.0f : absAxis, newState.axes[i], actionName);

    // For digital event, we treat action repeats like buttons and hats
    if (!CButtonTranslator::IsAnalog(actionID))
    {
      // NOW we overwrite old action and continue if no change in digital states
      bool bContinue = !((ABS(axes[i]) >= AXIS_DIGITAL_DEADZONE) ^ (absAxis >= AXIS_DIGITAL_DEADZONE));
      axes[i] = newState.axes[i];
      if (bContinue)
        continue;

      if (absAxis >= 0.01f) // Because we already sent a "centered" message
        CLog::Log(LOGDEBUG, "Joystick %d axis %d %s", id, i + 1,
          absAxis >= AXIS_DIGITAL_DEADZONE ? "activated" : "deactivated (but not centered)");

      if (!Wakeup() && absAxis >= AXIS_DIGITAL_DEADZONE)
      {
        g_application.ExecuteInputAction(action);
        m_actionTracker.Track(action);
      }
      else if (absAxis < AXIS_DIGITAL_DEADZONE)
      {
        m_actionTracker.Reset();
      }
    }
    else // CButtonTranslator::IsAnalog(actionID)
    {
      // We don't log about analog actions because they are sent every frame
      axes[i] = newState.axes[i];

      if (Wakeup())
        continue;

      if (newState.axes[i] != 0.0f)
        g_application.ExecuteInputAction(action);

      // The presence of analog actions disables others from being tracked
      m_actionTracker.Reset();
    }
  }
}

bool JoystickState::Wakeup()
{
  static bool bWokenUp = false;

  // Refresh bWokenUp after every call to ResetWakeup() (which sets m_bWakeupChecked to false)
  if (!m_bWakeupChecked)
  {
    m_bWakeupChecked = true;

    // Reset the timers and check to see if we have woken the application
    g_application.ResetSystemIdleTimer();
    g_application.ResetScreenSaver();
    bWokenUp = g_application.WakeUpScreenSaverAndDPMS();
  }
  return bWokenUp;
}
