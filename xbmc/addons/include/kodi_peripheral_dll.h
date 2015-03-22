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
#ifndef __PERIPHERAL_DLL_H__
#define __PERIPHERAL_DLL_H__

#include "kodi_peripheral_types.h"

#define PERIPHERAL_ADDON_JOYSTICKS // TODO

#ifdef __cplusplus
extern "C"
{
#endif

  /// @name Peripheral operations
  ///{
  /*!
   * @brief  Get the PERIPHERAL_API_VERSION used to compile this peripheral add-on
   * @return KODI_PERIPHERAL_API_VERSION from kodi_peripheral_types
   * @remarks Valid implementation required
   *
   * Used to check if the implementation is compatible with the frontend.
   */
  const char* GetPeripheralAPIVersion(void);

  /*!
   * @brief Get the KODI_PERIPHERAL_MIN_API_VERSION used to compile this peripheral add-on
   * @return KODI_PERIPHERAL_MIN_API_VERSION from kodi_peripheral_types
   * @remarks Valid implementation required
   *
   * Used to check if the implementation is compatible with the frontend.
   */
  const char* GetMinimumPeripheralAPIVersion(void);

  /*!
   * @brief Get the list of features that this add-on provides
   * @param pCapabilities The add-on's capabilities.
   * @return PERIPHERAL_NO_ERROR if the properties were fetched successfully.
   * @remarks Valid implementation required.
   *
   * Called by the frontend to query the add-on's capabilities and supported
   * peripherals. All capabilities that the add-on supports should be set to true.
   *
   */
  PERIPHERAL_ERROR GetAddonCapabilities(PERIPHERAL_CAPABILITIES *pCapabilities);

  /*!
   * @brief Perform a scan for joysticks
   * @param peripheral_count  Assigned to the number of peripherals allocated
   * @param scan_results      Assigned to allocated memory
   * @return PERIPHERAL_NO_ERROR if successful; peripherals must be freed using
   * FreeScanResults() in this case
   *
   * The frontend calls this when a hardware change is detected. If an add-on
   * detects a hardware change, it can trigger this function using the
   * TriggerScan() callback.
   */
  PERIPHERAL_ERROR PerformDeviceScan(unsigned int* peripheral_count, PERIPHERAL_INFO** scan_results);

  /*!
   * @brief Free the memory allocated in PerformDeviceScan()
   *
   * Must be called if PerformDeviceScan() returns PERIPHERAL_NO_ERROR.
   *
   * @param peripheral_count  The number of peripherals pointed to by scan_results
   * @param scan_results      The array of allocated peripherals
   */
  void FreeScanResults(unsigned int peripheral_count, PERIPHERAL_INFO* scan_results);

  /*!
   * @brief Get all events that have occurred since the last call to GetEvents()
   * @return PERIPHERAL_NO_ERROR if successful; events must be freed using
   * FreeEvents() in this case
   */
  PERIPHERAL_ERROR GetEvents(unsigned int* event_count, PERIPHERAL_EVENT** events);

  /*!
   * @brief Free the memory allocated in GetEvents()
   *
   * Must be called if GetEvents() returns PERIPHERAL_NO_ERROR.
   *
   * @param event_count  The number of events pointed to by events
   * @param events       The array of allocated events
   */
  void FreeEvents(unsigned int event_count, PERIPHERAL_EVENT* events);
  ///}

  /// @name Joystick operations
  /*!
   * @note #define PERIPHERAL_ADDON_JOYSTICKS before including kodi_peripheral_dll.h
   * in the add-on if the add-on provides joysticks and add provides_joysticks="true"
   * to the kodi.peripheral extension point node in addon.xml.
   */
  ///{
#ifdef PERIPHERAL_ADDON_JOYSTICKS
  /*!
   * @brief Get extended info about an attached joystick
   * @param index  The joystick's driver index
   * @param info   The container for the allocated joystick info
   * @return PERIPHERAL_NO_ERROR if successful; structs must be freed using
   * FreeJoystickInfo() in this case
   */
  PERIPHERAL_ERROR GetJoystickInfo(unsigned int index, JOYSTICK_INFO* info);

  /*!
   * @brief Free the memory allocated in GetJoystickInfo()
   */
  void FreeJoystickInfo(JOYSTICK_INFO* info);

  /*!
   * @brief Update joystick feature
   * @param index    The joystick's driver index
   * @param feature  The updated properties of the feature
   * @return PERIPHERAL_NO_ERROR if successful
   */
  PERIPHERAL_ERROR UpdateJoystickFeature(unsigned int index, JOYSTICK_FEATURE* feature);
#endif
  ///}

  /*!
   * Called by the frontend to assign the function pointers of this add-on to
   * pClient. Note that get_addon() is defined here, so it will be available in
   * all compiled peripheral add-ons.
   */
  void __declspec(dllexport) get_addon(struct PeripheralAddon* pClient)
  {
    pClient->GetPeripheralAPIVersion        = GetPeripheralAPIVersion;
    pClient->GetMinimumPeripheralAPIVersion = GetMinimumPeripheralAPIVersion;
    pClient->GetAddonCapabilities           = GetAddonCapabilities;
    pClient->PerformDeviceScan              = PerformDeviceScan;
    pClient->FreeScanResults                = FreeScanResults;

#ifdef PERIPHERAL_ADDON_JOYSTICKS
    pClient->GetJoystickInfo                = GetJoystickInfo;
    pClient->FreeJoystickInfo               = FreeJoystickInfo;
    pClient->GetEvents                      = GetEvents;
    pClient->FreeEvents                     = FreeEvents;
    pClient->UpdateJoystickFeature          = UpdateJoystickFeature;
#endif
  };

#ifdef __cplusplus
};
#endif

#endif // __PERIPHERAL_DLL_H__
