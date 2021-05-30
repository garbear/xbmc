/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/ImageSubscriptionKey.h"

#include <memory>
#include <string>

namespace PERIPHERALS
{
class CPeripheral;
}

namespace KODI
{
namespace SMART_HOME
{
class ISystemHealthHUD;
class IPowerMeterHUD;
class IVehicleHUD;

class IRos2
{
public:
  virtual ~IRos2() = default;

  /*!
   * \brief Initialize ROS 2 lifecycle
   */
  virtual void Initialize() = 0;

  /*!
   * \brief Deinitialize ROS 2 lifecycle
   */
  virtual void Deinitialize() = 0;

  /*!
   * \brief Look up a device by its ROS host name and scan address
   *
   * \param hostname The host component of the peer's peripherals topic
   * \param address The device address reported in the peer's peripheral scan
   *
   * \return The remote peripheral, or nullptr if the host or device has not
   *         been discovered or has disconnected
   */
  virtual std::shared_ptr<const PERIPHERALS::CPeripheral> GetRemotePeripheral(
      const std::string& hostname, const std::string& address) const = 0;

  // GUI interface
  virtual void RegisterImageTopic(const ImageSubscriptionKey& subscription) = 0;
  virtual void UnregisterImageTopic(const ImageSubscriptionKey& subscription) = 0;
  virtual ISystemHealthHUD* GetSystemHealthHUD() const = 0;
  virtual IPowerMeterHUD* GetPowerMeterHUD() const = 0;
  virtual IVehicleHUD* GetVehicleHUD() const = 0;

  //! @todo Remove GUI dependency
  virtual void FrameMove() = 0;
};
} // namespace SMART_HOME
} // namespace KODI
