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

#include "AddonCallbacksPeripheral.h"
#include "AddonCallbacks.h"
#include "include/kodi_peripheral_callbacks.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/log.h"

using namespace PERIPHERALS;

namespace ADDON
{

CAddonCallbacksPeripheral::CAddonCallbacksPeripheral(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_PeripheralLib;

  /* write XBMC peripheral specific add-on function addresses to callback table */
  m_callbacks->TriggerScan               = TriggerScan;
}

CAddonCallbacksPeripheral::~CAddonCallbacksPeripheral()
{
  /* delete the callback table */
  delete m_callbacks;
}

CPeripheralAddon* CAddonCallbacksPeripheral::GetPeripheralAddon(void* addonData, const char* strFunction)
{
  CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
  if (!addon || !addon->GetHelperPeripheral())
  {
    CLog::Log(LOGERROR, "PERIPHERAL - %s - called with a null pointer", strFunction);
    return NULL;
  }

  return dynamic_cast<CPeripheralAddon*>(addon->GetHelperPeripheral()->m_addon);
}

void CAddonCallbacksPeripheral::TriggerScan(void* addonData)
{
  g_peripherals.TriggerDeviceScan(PERIPHERAL_BUS_ADDON);
}

}; /* namespace ADDON */
