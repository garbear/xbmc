/*
 *  Copyright (C) 2023-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerActivity.h"

#include "input/InputTranslator.h"

using namespace KODI;
using namespace GAME;

#include <algorithm>
#include <cstdlib>

void CControllerActivity::OnButtonPress(bool pressed)
{
  if (pressed)
    m_currentActivation = 1.0f;
}

void CControllerActivity::OnButtonMotion(float magnitude)
{
  m_currentActivation = std::max(magnitude, m_currentActivation);
}

void CControllerActivity::OnAnalogStickMotion(float x, float y)
{
  m_currentActivation = std::max(std::abs(x), m_currentActivation);
  m_currentActivation = std::max(std::abs(y), m_currentActivation);
}

void CControllerActivity::OnWheelMotion(float position)
{
  m_currentActivation = std::max(std::abs(position), m_currentActivation);
}

void CControllerActivity::OnThrottleMotion(float position)
{
  m_currentActivation = std::max(std::abs(position), m_currentActivation);
}

void CControllerActivity::OnKeyPress(const KEYBOARD::KeyName& key)
{
  m_activeKeys.insert(key);
}

void CControllerActivity::OnKeyRelease(const KEYBOARD::KeyName& key)
{
  m_activeKeys.erase(key);
}

void CControllerActivity::OnMouseMotion(const MOUSE::PointerName& relpointer,
                                        int differenceX,
                                        int differenceY)
{
  //! @todo Handle multiple pointers
  //m_activePointers.insert(relpointer);

  INPUT::INTERCARDINAL_DIRECTION dir = GetPointerDirection(differenceX, differenceY);

  // Check if direction is valid
  if (dir != INPUT::INTERCARDINAL_DIRECTION::NONE)
    m_currentActivation = 1.0f;
}

void CControllerActivity::OnMouseButtonPress(const MOUSE::ButtonName& button)
{
  m_activeButtons.insert(button);
}

void CControllerActivity::OnMouseButtonRelease(const MOUSE::ButtonName& button)
{
  m_activeButtons.erase(button);
}

void CControllerActivity::OnInputFrame()
{
  // Process pressed keys
  if (!m_activeKeys.empty())
    m_currentActivation = 1.0f;

  // Process pressed mouse buttons
  if (!m_activeButtons.empty())
    m_currentActivation = 1.0f;

  // Process activation
  m_lastActivation = m_currentActivation;
  m_currentActivation = 0.0f;
}

INPUT::INTERCARDINAL_DIRECTION CControllerActivity::GetPointerDirection(int differenceX,
                                                                        int differenceY)
{
  using namespace INPUT;

  // Translate from left-handed coordinate system to right-handed coordinate system
  differenceY *= -1;

  return CInputTranslator::VectorToIntercardinalDirection(static_cast<float>(differenceX),
                                                          static_cast<float>(differenceY));
}
