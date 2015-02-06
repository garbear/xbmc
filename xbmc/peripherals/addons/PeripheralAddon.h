#pragma once
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

#include "addons/AddonDll.h"
#include "addons/DllPeripheral.h"
#include "addons/include/xbmc_peripheral_types.h"
#include "addons/include/xbmc_peripheral_utils.hpp"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/PeripheralTypes.h"
#include "threads/Thread.h"

#include <boost/shared_ptr.hpp>
#include <vector>

class CInputPrimitive;
class IJoystickInputHandler;

namespace PERIPHERALS
{
  class CPeripheral;

  class CPeripheralAddon;
  typedef boost::shared_ptr<CPeripheralAddon> PeripheralAddonPtr;
  typedef std::vector<PeripheralAddonPtr>     PeripheralAddonVector;

  typedef boost::shared_ptr<ADDON::JoystickFeature>       JoystickFeaturePtr;
  typedef std::map<JoystickFeatureID, JoystickFeaturePtr> JoystickFeatureMap;

  class CPeripheralAddon : public ADDON::CAddonDll<DllPeripheral, PeripheralAddon, PERIPHERAL_PROPERTIES>
  {
  public:
    CPeripheralAddon(const ADDON::AddonProps& props);
    CPeripheralAddon(const cp_extension_t *ext);
    virtual ~CPeripheralAddon(void);

    virtual ADDON::AddonPtr GetRunningInstance(void) const;

    /*!
     * @brief Initialise the instance of this add-on
     */
    ADDON_STATUS Create(void);

    /*!
     * @brief Destroy the instance of this add-on
     */
    void Destroy(void);

    bool         Register(unsigned int peripheralIndex, CPeripheral* peripheral);
    void         UnregisterRemovedDevices(const PeripheralScanResults &results, std::vector<CPeripheral*>& removedPeripherals);
    void         GetFeatures(std::vector<PeripheralFeature> &features) const;
    bool         HasFeature(const PeripheralFeature feature) const;
    CPeripheral* GetPeripheral(unsigned int index) const;
    CPeripheral* GetByPath(const std::string &strPath) const;
    int          GetPeripheralsWithFeature(std::vector<CPeripheral*> &results, const PeripheralFeature feature) const;
    size_t       GetNumberOfPeripherals(void) const;
    size_t       GetNumberOfPeripheralsWithId(const int iVendorId, const int iProductId) const;
    void         GetDirectory(const CStdString &strPath, CFileItemList &items) const;

    /** @name Peripheral add-on methods */
    //@{
    bool PerformDeviceScan(PeripheralScanResults &results);
    bool ProcessEvents(void);
    //@}

    /** @name Joystick methods */
    //@{
    bool GetJoystickFeatures(unsigned int index, JoystickFeatureMap& features);
    //@}

    static const char          *ToString(PERIPHERAL_ERROR error);

  protected:
    /*!
     * @brief Request the API version from the add-on, and check if it's compatible
     * @return True when compatible, false otherwise.
     */
    virtual bool CheckAPIVersion(void);

  private:
    static JoystickFeatureID    ToJoystickID(JOYSTICK_FEATURE_ID id);
    static JOYSTICK_FEATURE_ID ToFeatureID(JoystickFeatureID id);
    static HatDirection        ToHatDirection(JOYSTICK_STATE_HAT state);

    /*!
     * @brief Resets all class members to their defaults. Called by the constructors
     */
    void ResetProperties(void);

    bool GetAddonProperties(void);

    /*!
     * @brief Checks whether the provided API version is compatible with XBMC
     * @param minVersion The add-on's XBMC_PERIPHERAL_MIN_API_VERSION version
     * @param version The add-on's XBMC_PERIPHERAL_API_VERSION version
     * @return True when compatible, false otherwise
     */
    static bool IsCompatibleAPIVersion(const ADDON::AddonVersion &minVersion, const ADDON::AddonVersion &version);

    bool LogError(const PERIPHERAL_ERROR error, const char *strMethod) const;
    void LogException(const std::exception &e, const char *strFunctionName) const;

    /* stored strings to make sure const char* members in PERIPHERAL_PROPERTIES stay valid */
    std::string         m_strUserPath;    /*!< @brief translated path to the user profile */
    std::string         m_strClientPath;  /*!< @brief translated path to this add-on */

    /* add-on properties */
    ADDON::AddonVersion m_apiVersion;
    bool                m_bProvidesJoysticks;

    /* peripherals */
    std::map<unsigned int, CPeripheral*>  m_peripherals;

    /* synchronization */
    CCriticalSection    m_critSection;
  };
}
