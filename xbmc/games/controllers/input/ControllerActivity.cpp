/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerActivity.h"

using namespace KODI;
using namespace GAME;

#include <algorithm>
#include <cstdlib>

void CControllerActivity::OnButtonPress(bool pressed)
{
  if (pressed)
    m_currentActivity = 1.0f;
}

void CControllerActivity::OnButtonMotion(float magnitude)
{
  m_currentActivity = std::max(magnitude, m_currentActivity);
}

void CControllerActivity::OnAnalogStickMotion(float x, float y)
{
  m_currentActivity = std::max(std::abs(x), m_currentActivity);
  m_currentActivity = std::max(std::abs(y), m_currentActivity);
}

void CControllerActivity::OnWheelMotion(float position)
{
  m_currentActivity = std::max(std::abs(position), m_currentActivity);
}

void CControllerActivity::OnThrottleMotion(float position)
{
  m_currentActivity = std::max(std::abs(position), m_currentActivity);
}

void CControllerActivity::OnInputFrame()
{
  m_lastActivity = m_currentActivity;
  m_currentActivity = 0.0f;
}
