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

#include "PeripheralAddon.h"
#include "AddonJoystickButtonMap.h"
#include "addons/AddonManager.h"
#include "filesystem/SpecialProtocol.h"
#include "input/joysticks/InputPrimitive.h"
#include "peripherals/Peripherals.h"
#include "peripherals/bus/PeripheralBusAddon.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <string.h>

using namespace PERIPHERALS;

#ifndef SAFE_DELETE
  #define SAFE_DELETE(p)  do { delete (p); (p) = NULL; } while (0)
#endif

CPeripheralAddon::CPeripheralAddon(const ADDON::AddonProps& props)
 : CAddonDll<DllPeripheral, PeripheralAddon, PERIPHERAL_PROPERTIES>(props),
   m_apiVersion("0.0.0"),
   m_bProvidesJoysticks(false)
{
  ResetProperties();
}

CPeripheralAddon::CPeripheralAddon(const cp_extension_t *ext)
 : CAddonDll<DllPeripheral, PeripheralAddon, PERIPHERAL_PROPERTIES>(ext),
   m_apiVersion("0.0.0")
{
  ResetProperties();

  std::string strProvidesJoysticks = ADDON::CAddonMgr::Get().GetExtValue(ext->configuration, "@provides_joysticks");

  m_bProvidesJoysticks = StringUtils::EqualsNoCase(strProvidesJoysticks, "true");
}

CPeripheralAddon::~CPeripheralAddon(void)
{
  Destroy();
  SAFE_DELETE(m_pInfo);
}

void CPeripheralAddon::ResetProperties(void)
{
  /* initialise members */
  SAFE_DELETE(m_pInfo);
  m_pInfo = new PERIPHERAL_PROPERTIES;
  m_strUserPath        = CSpecialProtocol::TranslatePath(Profile());
  m_pInfo->user_path   = m_strUserPath.c_str();
  m_strClientPath      = CSpecialProtocol::TranslatePath(Path());
  m_pInfo->addon_path  = m_strClientPath.c_str();
  m_apiVersion = ADDON::AddonVersion("0.0.0");
  // TODO
}

ADDON::AddonPtr CPeripheralAddon::GetRunningInstance(void) const
{
  CPeripheralBusAddon* addonBus = static_cast<CPeripheralBusAddon*>(g_peripherals.GetBusByType(PERIPHERAL_BUS_ADDON));
  if (addonBus)
  {
    ADDON::AddonPtr peripheralAddon;
    if (addonBus->GetAddon(ID(), peripheralAddon))
      return peripheralAddon;
  }
  return CAddon::GetRunningInstance();
}

ADDON_STATUS CPeripheralAddon::Create(void)
{
  ADDON_STATUS status(ADDON_STATUS_UNKNOWN);

  /* ensure that a previous instance is destroyed */
  Destroy();

  /* reset all properties to defaults */
  ResetProperties();

  /* initialise the add-on */
  CLog::Log(LOGDEBUG, "PERIPHERAL - %s - creating peripheral add-on instance '%s'", __FUNCTION__, Name().c_str());
  try { status = CAddonDll<DllPeripheral, PeripheralAddon, PERIPHERAL_PROPERTIES>::Create(); }
  catch (const std::exception &e) { LogException(e, __FUNCTION__); }

  if (status == ADDON_STATUS_OK)
  {
    if (!GetAddonProperties())
    {
      Destroy();
      status = ADDON_STATUS_PERMANENT_FAILURE;
    }
  }

  return status;
}

void CPeripheralAddon::Destroy(void)
{
  /* reset 'ready to use' to false */
  CLog::Log(LOGDEBUG, "PERIPHERAL - %s - destroying peripheral add-on '%s'", __FUNCTION__, Name().c_str());

  /* destroy the add-on */
  CAddonDll<DllPeripheral, PeripheralAddon, PERIPHERAL_PROPERTIES>::Destroy();

  /* reset all properties to defaults */
  ResetProperties();
}

bool CPeripheralAddon::GetAddonProperties(void)
{
  PERIPHERAL_CAPABILITIES addonCapabilities = { };

  /* get the capabilities */
  try
  {
    PERIPHERAL_ERROR retVal = m_pStruct->GetAddonCapabilities(&addonCapabilities);
    if (retVal != PERIPHERAL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PERIPHERAL - couldn't get the capabilities for add-on '%s'. Please contact the developer of this add-on: %s",
          Name().c_str(), Author().c_str());
      return false;
    }
  }
  catch (std::exception &e) { LogException(e, "GetAddonCapabilities()"); return false; }

  // Verify capabilities against addon.xml
  if (m_bProvidesJoysticks != addonCapabilities.provides_joysticks)
  {
    CLog::Log(LOGERROR, "PERIPHERAL - Add-on '%s': provides_joysticks'(%s) in add-on DLL  doesn't match 'provides_joysticks'(%s) in addon.xml. Please contact the developer of this add-on: %s",
        Name().c_str(), addonCapabilities.provides_joysticks ? "true" : "false",
        m_bProvidesJoysticks ? "true" : "false", Author().c_str());
    return false;
  }

  return true;
}

bool CPeripheralAddon::CheckAPIVersion(void)
{
  /* check the API version */
  ADDON::AddonVersion minVersion = ADDON::AddonVersion(PERIPHERAL_MIN_API_VERSION);
  try { m_apiVersion = ADDON::AddonVersion(m_pStruct->GetPeripheralAPIVersion()); }
  catch (std::exception &e) { LogException(e, "GetPeripheralAPIVersion()"); return false; }

  if (!IsCompatibleAPIVersion(minVersion, m_apiVersion))
  {
    CLog::Log(LOGERROR, "PERIPHERAL - Add-on '%s' is using an incompatible API version. XBMC minimum API version = '%s', add-on API version '%s'", Name().c_str(), minVersion.asString().c_str(), m_apiVersion.asString().c_str());
    return false;
  }

  return true;
}

bool CPeripheralAddon::IsCompatibleAPIVersion(const ADDON::AddonVersion &minVersion, const ADDON::AddonVersion &version)
{
  ADDON::AddonVersion myMinVersion = ADDON::AddonVersion(PERIPHERAL_MIN_API_VERSION);
  ADDON::AddonVersion myVersion = ADDON::AddonVersion(PERIPHERAL_API_VERSION);
  return (version >= myMinVersion && minVersion <= myVersion);
}

bool CPeripheralAddon::Register(unsigned int peripheralIndex, CPeripheral *peripheral)
{
  if (!peripheral)
    return false;

  CSingleLock lock(m_critSection);

  if (m_peripherals.find(peripheralIndex) == m_peripherals.end())
  {
    if (peripheral->Type() == PERIPHERAL_JOYSTICK)
    {
      m_peripherals[peripheralIndex] = static_cast<CPeripheralJoystick*>(peripheral);

      CLog::Log(LOGNOTICE, "%s - new %s device registered on %s->%s: %s",
          __FUNCTION__, PeripheralTypeTranslator::TypeToString(peripheral->Type()),
          PeripheralTypeTranslator::BusTypeToString(PERIPHERAL_BUS_ADDON),
          peripheral->Location().c_str(), peripheral->DeviceName().c_str());

      return true;
    }
  }
  return false;
}

void CPeripheralAddon::UnregisterRemovedDevices(const PeripheralScanResults &results, std::vector<CPeripheral*>& removedPeripherals)
{
  CSingleLock lock(m_critSection);
  std::vector<unsigned int> removedIndexes;
  for (std::map<unsigned int, CPeripheral*>::const_iterator it = m_peripherals.begin(); it != m_peripherals.end(); ++it)
  {
    CPeripheral *peripheral = it->second;
    PeripheralScanResult updatedDevice(PERIPHERAL_BUS_ADDON);
    if (!results.GetDeviceOnLocation(peripheral->Location(), &updatedDevice) ||
      *peripheral != updatedDevice)
    {
      /* device removed */
      removedIndexes.push_back(it->first);
      //m_peripherals.erase(m_peripherals.begin() + iDevicePtr);
    }
  }
  lock.Leave();

  for (std::vector<unsigned int>::const_iterator it = removedIndexes.begin(); it != removedIndexes.end(); ++it)
  {
    CPeripheral *peripheral = m_peripherals[*it];
    CLog::Log(LOGNOTICE, "%s - device removed from %s/%s: %s (%s:%s)", __FUNCTION__, PeripheralTypeTranslator::TypeToString(peripheral->Type()), peripheral->Location().c_str(), peripheral->DeviceName().c_str(), peripheral->VendorIdAsString(), peripheral->ProductIdAsString());
    peripheral->OnDeviceRemoved();
    removedPeripherals.push_back(peripheral);
    m_peripherals.erase(*it);
  }
}

bool CPeripheralAddon::HasFeature(const PeripheralFeature feature) const
{
  if (feature == FEATURE_JOYSTICK)
    return m_bProvidesJoysticks;

  return false;
}

void CPeripheralAddon::GetFeatures(std::vector<PeripheralFeature> &features) const
{
  if (m_bProvidesJoysticks && std::find(features.begin(), features.end(), FEATURE_JOYSTICK) == features.end())
    features.push_back(FEATURE_JOYSTICK);
}

CPeripheral* CPeripheralAddon::GetPeripheral(unsigned int index) const
{
  CPeripheral* peripheral(NULL);
  CSingleLock lock(m_critSection);
  std::map<unsigned int, CPeripheral*>::const_iterator it = m_peripherals.find(index);
  if (it != m_peripherals.end())
    peripheral = it->second;
  return peripheral;
}

CPeripheral *CPeripheralAddon::GetByPath(const std::string &strPath) const
{
  CSingleLock lock(m_critSection);
  for (std::map<unsigned int, CPeripheral*>::const_iterator it = m_peripherals.begin(); it != m_peripherals.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(strPath, it->second->FileLocation()))
      return it->second;
  }

  return NULL;
}

int CPeripheralAddon::GetPeripheralsWithFeature(std::vector<CPeripheral*> &results, const PeripheralFeature feature) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);
  for (std::map<unsigned int, CPeripheral*>::const_iterator it = m_peripherals.begin(); it != m_peripherals.end(); ++it)
  {
    if (it->second->HasFeature(feature))
    {
      results.push_back(it->second);
      ++iReturn;
    }
  }
  return iReturn;
}

size_t CPeripheralAddon::GetNumberOfPeripherals(void) const
{
  CSingleLock lock(m_critSection);
  return m_peripherals.size();
}

size_t CPeripheralAddon::GetNumberOfPeripheralsWithId(const int iVendorId, const int iProductId) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);
  for (std::map<unsigned int, CPeripheral*>::const_iterator it = m_peripherals.begin(); it != m_peripherals.end(); ++it)
  {
    if (it->second->VendorId() == iVendorId &&
        it->second->ProductId() == iProductId)
      iReturn++;
  }

  return iReturn;
}

void CPeripheralAddon::GetDirectory(const CStdString &strPath, CFileItemList &items) const
{
  CStdString strDevPath;
  CSingleLock lock(m_critSection);
  for (std::map<unsigned int, CPeripheral*>::const_iterator it = m_peripherals.begin(); it != m_peripherals.end(); ++it)
  {
    const CPeripheral *peripheral = it->second;
    if (peripheral->IsHidden())
      continue;

    CFileItemPtr peripheralFile(new CFileItem(peripheral->DeviceName()));
    peripheralFile->SetPath(peripheral->FileLocation());
    peripheralFile->SetProperty("vendor", peripheral->VendorIdAsString());
    peripheralFile->SetProperty("product", peripheral->ProductIdAsString());
    peripheralFile->SetProperty("bus", PeripheralTypeTranslator::BusTypeToString(peripheral->GetBusType()));
    peripheralFile->SetProperty("location", peripheral->Location());
    peripheralFile->SetProperty("class", PeripheralTypeTranslator::TypeToString(peripheral->Type()));
    peripheralFile->SetProperty("version", peripheral->GetVersionInfo());
    items.Add(peripheralFile);
  }
}

bool CPeripheralAddon::PerformDeviceScan(PeripheralScanResults &results)
{
  unsigned int            peripheralCount;
  PERIPHERAL_INFO*        pScanResults;
  PERIPHERAL_ERROR        retVal;

  try { LogError(retVal = m_pStruct->PerformDeviceScan(&peripheralCount, &pScanResults), "PerformDeviceScan()"); }
  catch (std::exception &e) { LogException(e, "PerformDeviceScan()"); return false;  }

  if (retVal == PERIPHERAL_NO_ERROR)
  {
    for (unsigned int i = 0; i < peripheralCount; i++)
    {
      ADDON::Peripheral peripheral(pScanResults[i]);

      PeripheralScanResult result(PERIPHERAL_BUS_ADDON);
      switch (peripheral.Type())
      {
      case PERIPHERAL_TYPE_JOYSTICK:
        result.m_type = PERIPHERAL_JOYSTICK;
        break;
      default:
        continue;
      }

      result.m_strDeviceName = peripheral.Name();
      result.m_strLocation   = StringUtils::Format("%s/%d", ID().c_str(), peripheral.DriverIndex());
      result.m_iVendorId     = peripheral.VendorID();
      result.m_iProductId    = peripheral.ProductID();
      result.m_mappedType    = PERIPHERAL_JOYSTICK;
      result.m_mappedBusType = PERIPHERAL_BUS_ADDON;
      result.m_iSequence     = 0;

      if (!results.ContainsResult(result))
        results.m_results.push_back(result);
    }

    try { m_pStruct->FreeScanResults(peripheralCount, pScanResults); }
    catch (std::exception &e) { LogException(e, "FreeJoysticks()"); }

    return true;
  }

  return false;
}

bool CPeripheralAddon::ProcessEvents(void)
{
  if (!HasFeature(FEATURE_JOYSTICK))
    return false;

  PERIPHERAL_ERROR retVal;

  unsigned int      eventCount = 0;
  PERIPHERAL_EVENT* pEvents = NULL;

  try { LogError(retVal = m_pStruct->GetEvents(&eventCount, &pEvents), "GetEvents()"); }
  catch (std::exception &e) { LogException(e, "GetEvents()"); return false;  }

  if (retVal == PERIPHERAL_NO_ERROR && pEvents != NULL)
  {
    std::set<CPeripheralJoystick*> joysticksWithAxisMotion;

    for (unsigned int i = 0; i < eventCount; i++)
    {
      ADDON::PeripheralEvent event(pEvents[i]);
      CPeripheral* device = GetPeripheral(event.PeripheralIndex());
      if (!device)
        continue;

      switch (device->Type())
      {
      case PERIPHERAL_JOYSTICK:
      {
        CPeripheralJoystick* joystickDevice = static_cast<CPeripheralJoystick*>(device);

        switch (event.Type())
        {
          case PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON:
          {
            const bool bPressed = (event.ButtonState() == JOYSTICK_STATE_BUTTON_PRESSED);
            joystickDevice->OnButtonMotion(event.DriverIndex(), bPressed);
            break;
          }
          case PERIPHERAL_EVENT_TYPE_DRIVER_HAT:
          {
            joystickDevice->OnHatMotion(event.DriverIndex(), ToHatDirection(event.HatState()));
            break;
          }
          case PERIPHERAL_EVENT_TYPE_DRIVER_AXIS:
          {
            joystickDevice->OnAxisMotion(event.DriverIndex(), event.AxisState());
            joysticksWithAxisMotion.insert(joystickDevice);
            break;
          }
          default:
            break;
        }
        break;
      }
      default:
        break;
      }
    }

    for (std::set<CPeripheralJoystick*>::iterator it = joysticksWithAxisMotion.begin(); it != joysticksWithAxisMotion.end(); ++it)
      (*it)->ProcessAxisMotions();

    try { m_pStruct->FreeEvents(eventCount, pEvents); }
    catch (std::exception &e) { LogException(e, "FreeJoysticks()"); }

    return true;
  }

  return false;
}

bool CPeripheralAddon::GetJoystickFeatures(unsigned int index, JoystickFeatureMap& features)
{
  PERIPHERAL_ERROR retVal;


  JOYSTICK_INFO joystickStruct;

  try { LogError(retVal = m_pStruct->GetJoystickInfo(index, &joystickStruct), "GetJoystickInfo()"); }
  catch (std::exception &e) { LogException(e, "GetJoystickInfo()"); return false;  }

  if (retVal == PERIPHERAL_NO_ERROR)
  {
    ADDON::Joystick joystick(joystickStruct);

    for (std::vector<ADDON::JoystickFeature*>::const_iterator it = joystick.Features().begin(); it != joystick.Features().end(); ++it)
    {
      // Skip invalid features
      if (*it && ToJoystickID((*it)->ID()) && (*it)->Type())
        features[ToJoystickID((*it)->ID())] = JoystickFeaturePtr((*it)->Clone());
    }

    try { m_pStruct->FreeJoystickInfo(&joystickStruct); }
    catch (std::exception &e) { LogException(e, "FreeJoystickInfo()"); }

    return true;
  }

  return false;
}

const char *CPeripheralAddon::ToString(const PERIPHERAL_ERROR error)
{
  switch (error)
  {
  case PERIPHERAL_NO_ERROR:
    return "no error";
  case PERIPHERAL_ERROR_FAILED:
    return "command failed";
  case PERIPHERAL_ERROR_INVALID_PARAMETERS:
    return "invalid parameters";
  case PERIPHERAL_ERROR_NOT_IMPLEMENTED:
    return "not implemented";
  case PERIPHERAL_ERROR_NOT_CONNECTED:
    return "not connected";
  case PERIPHERAL_ERROR_CONNECTION_FAILED:
    return "connection failed";
  case PERIPHERAL_ERROR_UNKNOWN:
  default:
    return "unknown error";
  }
}

JoystickFeatureID CPeripheralAddon::ToJoystickID(JOYSTICK_FEATURE_ID id)
{
  switch (id)
  {
  case JOYSTICK_FEATURE_BUTTON_A:        return JOY_ID_BUTTON_A;
  case JOYSTICK_FEATURE_BUTTON_B:        return JOY_ID_BUTTON_B;
  case JOYSTICK_FEATURE_BUTTON_X:        return JOY_ID_BUTTON_X;
  case JOYSTICK_FEATURE_BUTTON_Y:        return JOY_ID_BUTTON_Y;
  case JOYSTICK_FEATURE_BUTTON_C:        return JOY_ID_BUTTON_C;
  case JOYSTICK_FEATURE_BUTTON_Z:        return JOY_ID_BUTTON_Z;
  case JOYSTICK_FEATURE_BUTTON_START:    return JOY_ID_BUTTON_START;
  case JOYSTICK_FEATURE_BUTTON_SELECT:   return JOY_ID_BUTTON_SELECT;
  case JOYSTICK_FEATURE_BUTTON_HOME:     return JOY_ID_BUTTON_MODE;
  case JOYSTICK_FEATURE_BUTTON_UP:       return JOY_ID_BUTTON_UP;
  case JOYSTICK_FEATURE_BUTTON_DOWN:     return JOY_ID_BUTTON_DOWN;
  case JOYSTICK_FEATURE_BUTTON_LEFT:     return JOY_ID_BUTTON_LEFT;
  case JOYSTICK_FEATURE_BUTTON_RIGHT:    return JOY_ID_BUTTON_RIGHT;
  case JOYSTICK_FEATURE_BUTTON_L:        return JOY_ID_BUTTON_L;
  case JOYSTICK_FEATURE_BUTTON_R:        return JOY_ID_BUTTON_R;
  case JOYSTICK_FEATURE_BUTTON_L_STICK:  return JOY_ID_BUTTON_L_STICK;
  case JOYSTICK_FEATURE_BUTTON_R_STICK:  return JOY_ID_BUTTON_R_STICK;
  case JOYSTICK_FEATURE_TRIGGER_L:       return JOY_ID_TRIGGER_L;
  case JOYSTICK_FEATURE_TRIGGER_R:       return JOY_ID_TRIGGER_R;
  case JOYSTICK_FEATURE_ANALOG_STICK_L:  return JOY_ID_ANALOG_STICK_L;
  case JOYSTICK_FEATURE_ANALOG_STICK_R:  return JOY_ID_ANALOG_STICK_R;
  case JOYSTICK_FEATURE_ACCELEROMETER:   return JOY_ID_ACCELEROMETER;
  default:                               return JOY_ID_BUTTON_UNKNOWN;
  }
}

JOYSTICK_FEATURE_ID CPeripheralAddon::ToFeatureID(JoystickFeatureID id)
{
  switch (id)
  {
  case JOY_ID_BUTTON_A:        return JOYSTICK_FEATURE_BUTTON_A;
  case JOY_ID_BUTTON_B:        return JOYSTICK_FEATURE_BUTTON_B;
  case JOY_ID_BUTTON_X:        return JOYSTICK_FEATURE_BUTTON_X;
  case JOY_ID_BUTTON_Y:        return JOYSTICK_FEATURE_BUTTON_Y;
  case JOY_ID_BUTTON_C:        return JOYSTICK_FEATURE_BUTTON_C;
  case JOY_ID_BUTTON_Z:        return JOYSTICK_FEATURE_BUTTON_Z;
  case JOY_ID_BUTTON_START:    return JOYSTICK_FEATURE_BUTTON_START;
  case JOY_ID_BUTTON_SELECT:   return JOYSTICK_FEATURE_BUTTON_SELECT;
  case JOY_ID_BUTTON_MODE:     return JOYSTICK_FEATURE_BUTTON_HOME;
  case JOY_ID_BUTTON_UP:       return JOYSTICK_FEATURE_BUTTON_UP;
  case JOY_ID_BUTTON_DOWN:     return JOYSTICK_FEATURE_BUTTON_DOWN;
  case JOY_ID_BUTTON_LEFT:     return JOYSTICK_FEATURE_BUTTON_LEFT;
  case JOY_ID_BUTTON_RIGHT:    return JOYSTICK_FEATURE_BUTTON_RIGHT;
  case JOY_ID_BUTTON_L:        return JOYSTICK_FEATURE_BUTTON_L;
  case JOY_ID_BUTTON_R:        return JOYSTICK_FEATURE_BUTTON_R;
  case JOY_ID_BUTTON_L_STICK:  return JOYSTICK_FEATURE_BUTTON_L_STICK;
  case JOY_ID_BUTTON_R_STICK:  return JOYSTICK_FEATURE_BUTTON_R_STICK;
  case JOY_ID_TRIGGER_L:       return JOYSTICK_FEATURE_TRIGGER_L;
  case JOY_ID_TRIGGER_R:       return JOYSTICK_FEATURE_TRIGGER_R;
  case JOY_ID_ANALOG_STICK_L:  return JOYSTICK_FEATURE_ANALOG_STICK_L;
  case JOY_ID_ANALOG_STICK_R:  return JOYSTICK_FEATURE_ANALOG_STICK_R;
  case JOY_ID_ACCELEROMETER:   return JOYSTICK_FEATURE_ACCELEROMETER;
  default:                     return JOYSTICK_FEATURE_UNKNOWN;
  }
}

HatDirection CPeripheralAddon::ToHatDirection(JOYSTICK_STATE_HAT state)
{
  switch (state)
  {
  case JOYSTICK_STATE_HAT_LEFT:   return HatDirectionLeft;
  case JOYSTICK_STATE_HAT_RIGHT:  return HatDirectionRight;
  case JOYSTICK_STATE_HAT_UP:     return HatDirectionUp;
  case JOYSTICK_STATE_HAT_DOWN:   return HatDirectionDown;
  default:                        return HatDirectionNone;
  }
}

bool CPeripheralAddon::LogError(const PERIPHERAL_ERROR error, const char *strMethod) const
{
  if (error != PERIPHERAL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "PERIPHERAL - %s - addon '%s' returned an error: %s",
        strMethod, Name().c_str(), ToString(error));
    return false;
  }
  return true;
}

void CPeripheralAddon::LogException(const std::exception &e, const char *strFunctionName) const
{
  CLog::Log(LOGERROR, "PERIPHERAL - exception '%s' caught while trying to call '%s' on add-on '%s'. Please contact the developer of this add-on: %s",
            e.what(), strFunctionName, Name().c_str(), Author().c_str());
}
