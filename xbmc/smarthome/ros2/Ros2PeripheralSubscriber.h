/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <oasis_msgs/msg/peripheral_info.hpp>

namespace PERIPHERALS
{
class CPeripheral;
class CPeripherals;
} // namespace PERIPHERALS

namespace KODI::SMART_HOME
{
class CRos2PeripheralSubscriber
{
public:
  explicit CRos2PeripheralSubscriber(PERIPHERALS::CPeripherals& manager);
  ~CRos2PeripheralSubscriber();

  void Update(const std::vector<oasis_msgs::msg::PeripheralInfo>& peripherals);
  std::shared_ptr<const PERIPHERALS::CPeripheral> GetPeripheral(const std::string& address) const;

  std::optional<int> GetControllerBattery() const;

private:
  class CRemotePeripheral;
  PERIPHERALS::CPeripherals& m_manager;
  mutable std::mutex m_mutex;
  std::optional<int> m_controllerBattery;
  std::map<std::string, std::shared_ptr<CRemotePeripheral>> m_peripherals;
};
} // namespace KODI::SMART_HOME
