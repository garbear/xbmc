/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2PeripheralSubscriber.h"

#include "peripherals/devices/Peripheral.h"

#include <set>
#include <string_view>

#include <oasis_msgs/msg/peripheral_constants.hpp>

using namespace KODI::SMART_HOME;
using namespace PERIPHERALS;

namespace
{
using Info = oasis_msgs::msg::PeripheralInfo;

PeripheralBattery ReadBattery(const Info& info)
{
  PeripheralBattery battery;
  if (info.battery_state == Info::BATTERY_STATE_WIRED)
    battery.state = PeripheralBatteryState::WIRED;
  else if (info.battery_state == Info::BATTERY_STATE_DISCHARGING)
    battery.state = PeripheralBatteryState::DISCHARGING;
  else if (info.battery_state == Info::BATTERY_STATE_CHARGING)
    battery.state = PeripheralBatteryState::CHARGING;
  else if (info.battery_state == Info::BATTERY_STATE_FULL)
    battery.state = PeripheralBatteryState::FULL;
  else if (info.battery_state != Info::BATTERY_STATE_UNKNOWN)
    return {};

  if (info.battery_charge >= 0 && info.battery_charge <= 100)
    battery.level = info.battery_charge;
  return battery;
}

PeripheralType ReadType(uint8_t type)
{
  using Constants = oasis_msgs::msg::PeripheralConstants;
  switch (type)
  {
    case Constants::TYPE_JOYSTICK:
      return PERIPHERAL_JOYSTICK;
    case Constants::TYPE_KEYBOARD:
      return PERIPHERAL_KEYBOARD;
    case Constants::TYPE_MOUSE:
      return PERIPHERAL_MOUSE;
    default:
      return PERIPHERAL_UNKNOWN;
  }
}

PeripheralScanResult ReadDevice(const Info& info)
{
  PeripheralScanResult result;
  result.m_mappedType = ReadType(info.type);
  result.m_strDeviceName = info.name;
  result.m_strLocation = info.address;
  result.m_iVendorId = info.vendor_id;
  result.m_iProductId = info.product_id;
  return result;
}
} // namespace

class CRos2PeripheralSubscriber::CRemotePeripheral : public CPeripheral
{
public:
  using CPeripheral::CPeripheral;

  PeripheralBattery GetBattery() const override
  {
    std::lock_guard lock(m_mutex);
    return m_battery;
  }

  void SetBattery(PeripheralBattery battery)
  {
    std::lock_guard lock(m_mutex);
    m_battery = battery;
  }

private:
  mutable std::mutex m_mutex;
  PeripheralBattery m_battery;
};

CRos2PeripheralSubscriber::CRos2PeripheralSubscriber(CPeripherals& manager) : m_manager(manager)
{
}

CRos2PeripheralSubscriber::~CRos2PeripheralSubscriber()
{
  Update({});
}

void CRos2PeripheralSubscriber::Update(const std::vector<Info>& peripherals)
{
  std::lock_guard lock(m_mutex);
  m_controllerBattery.reset();
  std::set<std::string_view> connected;
  for (const auto& info : peripherals)
  {
    if (info.address.empty())
      continue;

    connected.insert(info.address);
    auto& device = m_peripherals[info.address];
    if (!device || device->Type() != ReadType(info.type) || device->DeviceName() != info.name ||
        device->VendorId() != info.vendor_id || device->ProductId() != info.product_id)
    {
      if (device)
        device->SetBattery({});
      device = std::make_shared<CRemotePeripheral>(m_manager, ReadDevice(info), nullptr);
    }
    const auto battery = ReadBattery(info);
    device->SetBattery(battery);
    if (!m_controllerBattery && device->Type() == PERIPHERAL_JOYSTICK && battery.level >= 0)
      m_controllerBattery = battery.level;
  }
  std::erase_if(m_peripherals,
                [&connected](const auto& entry)
                {
                  if (connected.contains(entry.first))
                    return false;

                  entry.second->SetBattery({});
                  return true;
                });
}

std::shared_ptr<const CPeripheral> CRos2PeripheralSubscriber::GetPeripheral(
    const std::string& address) const
{
  std::lock_guard lock(m_mutex);
  const auto it = m_peripherals.find(address);
  return it != m_peripherals.end() ? it->second : nullptr;
}

std::optional<int> CRos2PeripheralSubscriber::GetControllerBattery() const
{
  std::lock_guard lock(m_mutex);
  return m_controllerBattery;
}
