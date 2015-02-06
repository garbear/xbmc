/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "PeripheralJoystick.h"
#include "input/joysticks/generic/GenericJoystickFeatureHandler.h"
#include "input/joysticks/generic/GenericJoystickInputHandler.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/AddonJoystickButtonMap.h"
#include "peripherals/bus/PeripheralBusAddon.h"
#include "utils/log.h"

using namespace PERIPHERALS;

CPeripheralJoystick::CPeripheralJoystick(const PeripheralScanResult& scanResult) :
  CPeripheral(scanResult),
  m_inputHandler(NULL),
  m_featureHandler(new CGenericJoystickFeatureHandler),
  m_buttonMap(NULL)
{
  m_features.push_back(FEATURE_JOYSTICK);
}

CPeripheralJoystick::~CPeripheralJoystick(void)
{
  delete m_inputHandler;
  delete m_featureHandler;
  delete m_buttonMap;
}

bool CPeripheralJoystick::InitialiseFeature(const PeripheralFeature feature)
{
  if (!CPeripheral::InitialiseFeature(feature))
    return false;

  if (feature == FEATURE_JOYSTICK)
  {
    if (m_mappedBusType == PERIPHERAL_BUS_ADDON)
    {
      CPeripheralBusAddon* addonBus = static_cast<CPeripheralBusAddon*>(g_peripherals.GetBusByType(PERIPHERAL_BUS_ADDON));
      if (addonBus)
      {
        PeripheralAddonPtr addon;
        unsigned int index;
        if (addonBus->SplitLocation(m_strLocation, addon, index))
        {
          m_buttonMap = new CAddonJoystickButtonMap(addon, index);
          if (m_buttonMap->Load())
            m_inputHandler  = new CGenericJoystickInputHandler(m_featureHandler, m_buttonMap);
        }
        else
          CLog::Log(LOGERROR, "CPeripheralJoystick: Invalid location (%s)", m_strLocation.c_str());
      }
    }
  }

  return m_inputHandler != NULL;
}

void CPeripheralJoystick::OnButtonMotion(unsigned int index, bool bPressed)
{
  if (m_inputHandler)
    m_inputHandler->OnButtonMotion(index, bPressed);
}

void CPeripheralJoystick::OnHatMotion(unsigned int index, HatDirection direction)
{
  if (m_inputHandler)
    m_inputHandler->OnHatMotion(index, direction);
}

void CPeripheralJoystick::OnAxisMotion(unsigned int index, float position)
{
  if (m_inputHandler)
    m_inputHandler->OnAxisMotion(index, position);
}

void CPeripheralJoystick::ProcessAxisMotions()
{
  if (m_inputHandler)
    m_inputHandler->ProcessAxisMotions();
}
