/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidJoystickState.h"

#include "AndroidJoystickTranslator.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>
#include <utility>

#include <android/input.h>
#include <androidjni/View.h>

using namespace PERIPHERALS;

static std::string PrintAxisIds(const std::vector<int>& axisIds)
{
  if (axisIds.empty())
    return "";

  if (axisIds.size() == 1)
    return std::to_string(axisIds.front());

  std::string strAxisIds;
  for (const auto& axisId : axisIds)
  {
    if (strAxisIds.empty())
      strAxisIds = "[";
    else
      strAxisIds += " | ";

    strAxisIds += std::to_string(axisId);
  }
  strAxisIds += "]";

  return strAxisIds;
}

static void MapAxisIds(int axisId,
                       int primaryAxisId,
                       int secondaryAxisId,
                       std::vector<int>& axisIds)
{
  if (axisId != primaryAxisId && axisId != secondaryAxisId)
    return;

  if (axisIds.empty())
  {
    axisIds.emplace_back(primaryAxisId);
    axisIds.emplace_back(secondaryAxisId);
  }

  if (axisIds.size() > 1)
    return;

  if (axisId == primaryAxisId)
    axisIds.emplace_back(secondaryAxisId);
  else if (axisId == secondaryAxisId)
    axisIds.insert(axisIds.begin(), primaryAxisId);
}

CAndroidJoystickState::CAndroidJoystickState(CAndroidJoystickState&& other) noexcept
  : m_deviceId(other.m_deviceId),
    m_buttons(std::move(other.m_buttons)),
    m_axes(std::move(other.m_axes)),
    m_analogState(std::move(other.m_analogState)),
    m_digitalEvents(std::move(other.m_digitalEvents))
{
}

CAndroidJoystickState::~CAndroidJoystickState()
{
  Deinitialize();
}

bool CAndroidJoystickState::Initialize(const CJNIViewInputDevice& inputDevice)
{
  if (!inputDevice)
    return false;

  const std::string deviceName = inputDevice.getName();

  // get the device ID
  m_deviceId = inputDevice.getId();

  // get all motion ranges to be able to count the number of available buttons, hats and axis'
  const CJNIList<CJNIViewInputDeviceMotionRange> motionRanges = inputDevice.getMotionRanges();
  for (int index = 0; index < motionRanges.size(); ++index)
  {
    const CJNIViewInputDeviceMotionRange motionRange = motionRanges.get(index);
    if (!motionRange.isFromSource(CJNIViewInputDevice::SOURCE_JOYSTICK) &&
        !motionRange.isFromSource(CJNIViewInputDevice::SOURCE_GAMEPAD))
    {
      CLog::Log(LOGDEBUG,
                "CAndroidJoystickState: axis {} has unexpected source {} ({}) for input device "
                "\"{}\" with ID {}",
                motionRange.getAxis(),
                CAndroidJoystickTranslator::TranslateSource(motionRange.getSource()),
                motionRange.getSource(), deviceName, m_deviceId);
    }

    int axisId = motionRange.getAxis();
    JoystickAxis axis{{axisId},
                      motionRange.getFlat(),
                      motionRange.getFuzz(),
                      motionRange.getMin(),
                      motionRange.getMax(),
                      motionRange.getRange(),
                      motionRange.getResolution()};

    // check if the axis ID belongs to a D-pad, analogue stick, trigger or
    // generic axis
    // clang-format off
    if (axisId == AMOTION_EVENT_AXIS_HAT_X ||
        axisId == AMOTION_EVENT_AXIS_HAT_Y ||
        axisId == AMOTION_EVENT_AXIS_X ||
        axisId == AMOTION_EVENT_AXIS_Y ||
        axisId == AMOTION_EVENT_AXIS_Z ||
        axisId == AMOTION_EVENT_AXIS_RX ||
        axisId == AMOTION_EVENT_AXIS_RY ||
        axisId == AMOTION_EVENT_AXIS_RZ ||
        axisId == AMOTION_EVENT_AXIS_LTRIGGER ||
        axisId == AMOTION_EVENT_AXIS_RTRIGGER ||
        axisId == AMOTION_EVENT_AXIS_GAS ||
        axisId == AMOTION_EVENT_AXIS_BRAKE ||
        axisId == AMOTION_EVENT_AXIS_THROTTLE ||
        axisId == AMOTION_EVENT_AXIS_RUDDER ||
        axisId == AMOTION_EVENT_AXIS_WHEEL ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_1 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_2 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_3 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_4 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_5 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_6 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_7 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_8 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_9 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_10 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_11 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_12 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_13 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_14 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_15 ||
        axisId == AMOTION_EVENT_AXIS_GENERIC_16)
    // clang-format on
    {
      // check if this axis is already known
      if (ContainsAxis(axisId, m_axes))
      {
        CLog::Log(LOGWARNING,
                  "CAndroidJoystickState: duplicate axis {} on input device \"{}\" with ID {}",
                  PrintAxisIds(axis.ids), deviceName, m_deviceId);
        continue;
      }

      // map AMOTION_EVENT_AXIS_GAS to AMOTION_EVENT_AXIS_RTRIGGER and
      // AMOTION_EVENT_AXIS_BRAKE to AMOTION_EVENT_AXIS_LTRIGGER
      // to avoid duplicate events on controllers sending both events
      MapAxisIds(axisId, AMOTION_EVENT_AXIS_LTRIGGER, AMOTION_EVENT_AXIS_BRAKE, axis.ids);
      MapAxisIds(axisId, AMOTION_EVENT_AXIS_RTRIGGER, AMOTION_EVENT_AXIS_GAS, axis.ids);

      m_axes.emplace_back(std::move(axis));
      CLog::Log(LOGDEBUG,
                "CAndroidJoystickState: axis {} on input device \"{}\" with ID {} detected",
                PrintAxisIds(m_axes.back().ids), deviceName, m_deviceId);
    }
    else
      CLog::Log(LOGWARNING,
                "CAndroidJoystickState: ignoring unknown axis {} on input device \"{}\" with ID {}",
                axisId, deviceName, m_deviceId);
  }

  // clang-format off
  std::vector<int> buttons{
      // add the usual suspects
      AKEYCODE_BUTTON_A,
      AKEYCODE_BUTTON_B,
      AKEYCODE_BUTTON_C,
      AKEYCODE_BUTTON_X,
      AKEYCODE_BUTTON_Y,
      AKEYCODE_BUTTON_Z,
      AKEYCODE_BACK,
      AKEYCODE_MENU,
      AKEYCODE_HOME,
      AKEYCODE_BUTTON_SELECT,
      AKEYCODE_BUTTON_MODE,
      AKEYCODE_BUTTON_START,
      AKEYCODE_BUTTON_L1,
      AKEYCODE_BUTTON_R1,
      AKEYCODE_BUTTON_L2,
      AKEYCODE_BUTTON_R2,
      AKEYCODE_BUTTON_THUMBL,
      AKEYCODE_BUTTON_THUMBR,
      AKEYCODE_DPAD_UP,
      AKEYCODE_DPAD_RIGHT,
      AKEYCODE_DPAD_DOWN,
      AKEYCODE_DPAD_LEFT,
      AKEYCODE_DPAD_CENTER,
      // found on the NVidia Shield controller
      AKEYCODE_VOLUME_UP,
      AKEYCODE_VOLUME_DOWN,
      AKEYCODE_MEDIA_PLAY_PAUSE,
      // add generic gamepad buttons for controllers that Android doesn't know
      // how to map
      AKEYCODE_BUTTON_1,
      AKEYCODE_BUTTON_2,
      AKEYCODE_BUTTON_3,
      AKEYCODE_BUTTON_4,
      AKEYCODE_BUTTON_5,
      AKEYCODE_BUTTON_6,
      AKEYCODE_BUTTON_7,
      AKEYCODE_BUTTON_8,
      AKEYCODE_BUTTON_9,
      AKEYCODE_BUTTON_10,
      AKEYCODE_BUTTON_11,
      AKEYCODE_BUTTON_12,
      AKEYCODE_BUTTON_13,
      AKEYCODE_BUTTON_14,
      AKEYCODE_BUTTON_15,
      AKEYCODE_BUTTON_16,
  };
  // clang-format on

  // check for presence of buttons
  auto results = inputDevice.hasKeys(buttons);

  if (results.size() != buttons.size())
  {
    CLog::Log(LOGERROR, "CAndroidJoystickState: failed to get key status for {} buttons",
              buttons.size());
    return false;
  }

  // log positive results and assign results to buttons
  for (unsigned int i = 0; i < buttons.size(); ++i)
  {
    if (results[i])
    {
      CLog::Log(LOGDEBUG, "CAndroidJoystickState:     button found: {} ({})",
                CAndroidJoystickTranslator::TranslateKeyCode(buttons[i]), buttons[i]);
      m_buttons.emplace_back(JoystickAxis{{buttons[i]}});
    }
  }

  // check if there are no buttons or axes at all
  if (GetButtonCount() == 0 && GetAxisCount() == 0)
  {
    CLog::Log(LOGWARNING,
              "CAndroidJoystickState: no buttons, hats or axes detected for input device \"{}\" "
              "with ID {}",
              deviceName, m_deviceId);
    return false;
  }

  m_analogState.assign(GetAxisCount(), 0.0f);

  return true;
}

void CAndroidJoystickState::Deinitialize(void)
{
  m_buttons.clear();
  m_axes.clear();

  m_analogState.clear();
  m_digitalEvents.clear();
}

bool CAndroidJoystickState::ProcessEvent(const AInputEvent* event)
{
  int32_t type = AInputEvent_getType(event);

  switch (type)
  {
    case AINPUT_EVENT_TYPE_KEY:
    {
      int32_t keycode = AKeyEvent_getKeyCode(event);
      int32_t action = AKeyEvent_getAction(event);

      JOYSTICK_STATE_BUTTON buttonState = JOYSTICK_STATE_BUTTON_UNPRESSED;
      if (action == AKEY_EVENT_ACTION_DOWN)
        buttonState = JOYSTICK_STATE_BUTTON_PRESSED;

      CLog::Log(LOGDEBUG, "Android Key {} ({}) {}",
                CAndroidJoystickTranslator::TranslateKeyCode(keycode), keycode,
                (buttonState == JOYSTICK_STATE_BUTTON_UNPRESSED ? "released" : "pressed"));

      bool result = SetButtonValue(keycode, buttonState);

      return result;
    }

    case AINPUT_EVENT_TYPE_MOTION:
    {
      size_t count = AMotionEvent_getPointerCount(event);

      bool success = false;
      for (size_t pointer = 0; pointer < count; ++pointer)
      {
        // process all axes
        for (const auto& axis : m_axes)
        {
          // get all potential values
          std::vector<float> values;
          for (const auto& axisId : axis.ids)
            values.emplace_back(AMotionEvent_getAxisValue(event, axisId, pointer));

          // remove all zero values
          values.erase(std::remove(values.begin(), values.end(), 0.0f), values.end());

          float value = 0.0f;
          // pick the first non-zero value
          if (!values.empty())
            value = values.front();

          success |= SetAxisValue(axis.ids, value);
        }
      }
      return success;
    }

    default:
      CLog::Log(LOGWARNING,
                "CAndroidJoystickState: unknown input event type {} from input device with ID {}",
                type, m_deviceId);
      break;
  }

  return false;
}

void CAndroidJoystickState::GetEvents(std::vector<kodi::addon::PeripheralEvent>& events)
{
  GetButtonEvents(events);
  GetAxisEvents(events);
}

void CAndroidJoystickState::GetButtonEvents(std::vector<kodi::addon::PeripheralEvent>& events)
{
  std::unique_lock<CCriticalSection> lock(m_eventMutex);

  // Only report a single event per button (avoids dropping rapid presses)
  std::vector<kodi::addon::PeripheralEvent> repeatButtons;

  for (const auto& digitalEvent : m_digitalEvents)
  {
    auto HasButton = [&digitalEvent](const kodi::addon::PeripheralEvent& event)
    {
      if (event.Type() == PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON)
        return event.DriverIndex() == digitalEvent.DriverIndex();
      return false;
    };

    if (std::find_if(events.begin(), events.end(), HasButton) == events.end())
      events.emplace_back(digitalEvent);
    else
      repeatButtons.emplace_back(digitalEvent);
  }

  m_digitalEvents.swap(repeatButtons);
}

void CAndroidJoystickState::GetAxisEvents(std::vector<kodi::addon::PeripheralEvent>& events) const
{
  for (unsigned int i = 0; i < m_analogState.size(); i++)
    events.emplace_back(m_deviceId, i, m_analogState[i]);
}

bool CAndroidJoystickState::SetButtonValue(int axisId, JOYSTICK_STATE_BUTTON buttonValue)
{
  size_t buttonIndex = 0;
  if (!GetAxesIndex({axisId}, m_buttons, buttonIndex) || buttonIndex >= GetButtonCount())
    return false;

  std::unique_lock<CCriticalSection> lock(m_eventMutex);

  m_digitalEvents.emplace_back(m_deviceId, buttonIndex, buttonValue);

  return true;
}

bool CAndroidJoystickState::SetAxisValue(const std::vector<int>& axisIds,
                                         JOYSTICK_STATE_AXIS axisValue)
{
  size_t axisIndex = 0;
  if (!GetAxesIndex(axisIds, m_axes, axisIndex) || axisIndex >= GetAxisCount())
    return false;

  const JoystickAxis& axis = m_axes[axisIndex];

  // make sure that the axis value is in the valid range
  axisValue = Contain(axisValue, axis.min, axis.max);
  // apply deadzoning
  axisValue = Deadzone(axisValue, axis.flat);
  // scale the axis value down to a value between -1.0f and 1.0f
  axisValue = Scale(axisValue, axis.max, 1.0f);

  m_analogState[axisIndex] = axisValue;
  return true;
}

float CAndroidJoystickState::Contain(float value, float min, float max)
{
  if (value < min)
    return min;
  if (value > max)
    return max;

  return value;
}

float CAndroidJoystickState::Scale(float value, float max, float scaledMax)
{
  return value * (scaledMax / max);
}

float CAndroidJoystickState::Deadzone(float value, float deadzone)
{
  if ((value > 0.0f && value < deadzone) || (value < 0.0f && value > -deadzone))
    return 0.0f;

  return value;
}

CAndroidJoystickState::JoystickAxes::const_iterator CAndroidJoystickState::GetAxis(
    const std::vector<int>& axisIds, const JoystickAxes& axes)
{
  return std::find_if(axes.cbegin(), axes.cend(),
                      [&axisIds](const JoystickAxis& axis)
                      {
                        std::vector<int> matches(std::max(axisIds.size(), axis.ids.size()));
                        const auto& matchesEnd =
                            std::set_intersection(axisIds.begin(), axisIds.end(), axis.ids.begin(),
                                                  axis.ids.end(), matches.begin());
                        matches.resize(matchesEnd - matches.begin());
                        return !matches.empty();
                      });
}

bool CAndroidJoystickState::ContainsAxis(int axisId, const JoystickAxes& axes)
{
  return GetAxis({axisId}, axes) != axes.cend();
}

bool CAndroidJoystickState::GetAxesIndex(const std::vector<int>& axisIds,
                                         const JoystickAxes& axes,
                                         size_t& axesIndex)
{
  auto axesIt = GetAxis(axisIds, axes);
  if (axesIt == axes.end())
    return false;

  axesIndex = std::distance(axes.begin(), axesIt);
  return true;
}
