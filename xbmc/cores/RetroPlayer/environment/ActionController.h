/*
 *      Copyright (C) 2018 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <vector>

namespace KODI
{
namespace RETRO
{
  struct DigitalButtonState
  {
    bool pressed;
  };

  struct AnalogButtonState
  {
    float magnitude;
  };

  struct AnalogStickState
  {
    float x;
    float y;
  };

  struct AccelerometerState
  {
    float x;
    float y;
    float z;
  };

  struct RelativePointerState
  {
    int x;
    int y;
  };

  struct AbsolutePointerState
  {
    bool pressed;
    float x;
    float y;
  };

  struct WheelState
  {
    float position;
  };

  struct ThrottleState
  {
    float position;
  };

  class CControllerState
  {
  public:
    CControllerState();
    ~CControllerState();

    std::vector<DigitalButtonState> m_digitalButtons;
    std::vector<AnalogButtonState> m_analogButtons;
    std::vector<AnalogStickState> m_analogSticks;
    std::vector<AccelerometerState> m_accelerometers;
    std::vector<RelativePointerState> m_relativePointers;
    std::vector<AbsolutePointerState> m_absolutePointers;
    std::vector<WheelState> m_wheels;
    std::vector<ThrottleState> m_throttles;
  };
}
}
