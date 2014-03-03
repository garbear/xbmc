/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "GameClientInput.h"
#include "GameClient.h"
#include "games/controllers/Controller.h"

#include <algorithm>
#include <assert.h>

using namespace GAME;

CGameClientInput::CGameClientInput(CGameClient* addon, int port, const ControllerPtr& controller) :
  m_addon(addon),
  m_port(port),
  m_controller(controller)
{
  assert(m_addon != NULL);
  assert(controller.get() != NULL);
}

std::string CGameClientInput::ControllerID(void) const
{
  return m_controller->ID();
}

bool CGameClientInput::HasFeature(const std::string& feature) const
{
  return m_addon->HasFeature(m_controller->ID(), feature);
}

JOYSTICK::INPUT_TYPE CGameClientInput::GetInputType(const std::string& feature) const
{
  const std::vector<CControllerFeature>& features = m_controller->Layout().Features();

  for (std::vector<CControllerFeature>::const_iterator it = features.begin(); it != features.end(); ++it)
  {
    if (feature == it->Name())
      return it->InputType();
  }

  return JOYSTICK::INPUT_TYPE::UNKNOWN;
}

bool CGameClientInput::OnButtonPress(const std::string& feature, bool bPressed)
{
  return m_addon->OnButtonPress(m_port, feature, bPressed);
}

bool CGameClientInput::OnButtonMotion(const std::string& feature, float magnitude)
{
  return m_addon->OnButtonMotion(m_port, feature, magnitude);
}

bool CGameClientInput::OnAnalogStickMotion(const std::string& feature, float x, float y)
{
  return m_addon->OnAnalogStickMotion(m_port, feature, x, y);
}

bool CGameClientInput::OnAccelerometerMotion(const std::string& feature, float x, float y, float z)
{
  return m_addon->OnAccelerometerMotion(m_port, feature, x, y, z);
}
