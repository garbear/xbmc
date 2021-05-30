/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "smarthome/ros2/Ros2Translator.h"

#include <utility>

#include <gtest/gtest.h>
#include <oasis_msgs/msg/peripheral_info.hpp>

using namespace PERIPHERALS;
using KODI::SMART_HOME::CRos2Translator;
using PeripheralInfo = oasis_msgs::msg::PeripheralInfo;

TEST(TestRos2PeripheralBattery, PublishesCachedJoystickReadings)
{
  PeripheralInfo info;
  for (const auto& [state, expected] :
       {std::pair{PeripheralBatteryState::UNKNOWN, "UNKNOWN"},
        std::pair{PeripheralBatteryState::WIRED, "WIRED"},
        std::pair{PeripheralBatteryState::DISCHARGING, "DISCHARGING"},
        std::pair{PeripheralBatteryState::CHARGING, "CHARGING"},
        std::pair{PeripheralBatteryState::FULL, "FULL"}})
  {
    for (int level : {-1, 0, 42, 100})
    {
      CRos2Translator::SetPeripheralBattery(info, {state, level}, PERIPHERAL_BUS_ADDON);
      EXPECT_EQ(info.battery_state, expected);
      EXPECT_EQ(info.battery_charge, level);
    }
  }
}

TEST(TestRos2PeripheralBattery, IdentifiesDevicesWithoutBatteryReporting)
{
  PeripheralInfo info;
  CRos2Translator::SetPeripheralBattery(info, {}, PERIPHERAL_BUS_APPLICATION);
  EXPECT_EQ(info.battery_state, "NOT_SUPPORTED");
  EXPECT_EQ(info.battery_charge, -1);

  CRos2Translator::SetPeripheralBattery(info, {PeripheralBatteryState::UNKNOWN, 25},
                                        PERIPHERAL_BUS_APPLICATION);
  EXPECT_EQ(info.battery_state, PeripheralInfo::BATTERY_STATE_UNKNOWN);
  EXPECT_EQ(info.battery_charge, 25);

  for (auto bus : {PERIPHERAL_BUS_USB, PERIPHERAL_BUS_CEC})
  {
    CRos2Translator::SetPeripheralBattery(info, {}, bus);
    EXPECT_EQ(info.battery_state, PeripheralInfo::BATTERY_STATE_UNKNOWN);
    EXPECT_EQ(info.battery_charge, -1);
  }
}
