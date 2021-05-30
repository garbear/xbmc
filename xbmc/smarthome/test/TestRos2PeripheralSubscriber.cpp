/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "games/controllers/ControllerManager.h"
#include "input/InputManager.h"
#include "interfaces/AnnouncementManager.h"
#include "peripherals/Peripherals.h"
#include "smarthome/ros2/Ros2PeripheralSubscriber.h"

#include <string>

#include <gtest/gtest.h>
#include <oasis_msgs/msg/peripheral_constants.hpp>

using namespace KODI::SMART_HOME;
using namespace PERIPHERALS;
using Info = oasis_msgs::msg::PeripheralInfo;

class TestRos2PeripheralSubscriber : public testing::Test
{
protected:
  void SetUp() override
  {
    m_previous = CServiceBroker::GetAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(
        std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
  }

  void TearDown() override { CServiceBroker::RegisterAnnouncementManager(std::move(m_previous)); }

  CInputManager m_input;
  KODI::GAME::CControllerManager m_controllers{CServiceBroker::GetAddonMgr()};

private:
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previous;
};

TEST_F(TestRos2PeripheralSubscriber, AppliesBatteryReadingsToTheConnectedDevice)
{
  CPeripherals manager(m_input, m_controllers);
  CRos2PeripheralSubscriber subscriber(manager);
  Info info;
  info.address = "device";
  info.type = oasis_msgs::msg::PeripheralConstants::TYPE_KEYBOARD;
  subscriber.Update({info});
  auto peripheral = subscriber.GetPeripheral(info.address);
  ASSERT_TRUE(peripheral);
  EXPECT_EQ(PERIPHERAL_KEYBOARD, peripheral->Type());

  const auto check = [&](const std::string& state, int charge, PeripheralBattery expected)
  {
    info.battery_state = state;
    info.battery_charge = charge;
    subscriber.Update({info});
    EXPECT_EQ(peripheral, subscriber.GetPeripheral(info.address));
    EXPECT_EQ(expected, peripheral->GetBattery());
  };
  check(Info::BATTERY_STATE_DISCHARGING, 0, {PeripheralBatteryState::DISCHARGING, 0});
  check(Info::BATTERY_STATE_CHARGING, 58, {PeripheralBatteryState::CHARGING, 58});
  check(Info::BATTERY_STATE_FULL, 100, {PeripheralBatteryState::FULL, 100});
  check(Info::BATTERY_STATE_WIRED, -1, {PeripheralBatteryState::WIRED, -1});
  check(Info::BATTERY_STATE_UNKNOWN, -1, {});
  check(Info::BATTERY_STATE_UNKNOWN, 25, {PeripheralBatteryState::UNKNOWN, 25});
  check(Info::BATTERY_STATE_NOT_SUPPORTED, 0, {});
  check(Info::BATTERY_STATE_CHARGING, -2, {PeripheralBatteryState::CHARGING, -1});
  check(Info::BATTERY_STATE_DISCHARGING, 101, {PeripheralBatteryState::DISCHARGING, -1});
  check("UNRECOGNIZED", 30, {});
}

TEST_F(TestRos2PeripheralSubscriber, RemovesAndRecreatesDevicesFromFullScans)
{
  CPeripherals manager(m_input, m_controllers);
  CRos2PeripheralSubscriber subscriber(manager);
  Info info;
  info.address = "pad";
  info.battery_state = Info::BATTERY_STATE_DISCHARGING;
  info.battery_charge = 45;
  subscriber.Update({info});
  auto disconnected = subscriber.GetPeripheral(info.address);
  ASSERT_TRUE(disconnected);

  subscriber.Update({});
  EXPECT_FALSE(subscriber.GetPeripheral(info.address));
  EXPECT_EQ(PeripheralBattery{}, disconnected->GetBattery());

  info.battery_state = Info::BATTERY_STATE_UNKNOWN;
  info.battery_charge = -1;
  subscriber.Update({info});
  auto reconnected = subscriber.GetPeripheral(info.address);
  ASSERT_TRUE(reconnected);
  EXPECT_NE(disconnected, reconnected);
  EXPECT_EQ(PeripheralBattery{}, reconnected->GetBattery());
}
