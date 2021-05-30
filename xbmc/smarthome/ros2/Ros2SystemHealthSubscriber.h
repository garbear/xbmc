/*
 *  Copyright (C) 2024-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Temperature.h"

#include <chrono>
#include <mutex>
#include <optional>

#include <oasis_msgs/msg/system_telemetry.hpp>
#include <oasis_msgs/msg/ups_status.hpp>
#include <rclcpp/subscription.hpp>

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
/*!
 * \brief ROS 2 system health subscriber
 */
class CRos2SystemHealthSubscriber
{
public:
  CRos2SystemHealthSubscriber(std::string rosNamespace);
  CRos2SystemHealthSubscriber(const CRos2SystemHealthSubscriber&) = delete;
  CRos2SystemHealthSubscriber(CRos2SystemHealthSubscriber&&) = delete;

  // ROS functions
  void Initialize(std::shared_ptr<rclcpp::Node> node, const std::string& systemName);
  void Deinitialize();

  // GUI functions
  bool IsActive(std::chrono::milliseconds timeout) const;
  CTemperature CPUTemperature() const;
  std::optional<float> CPUUtilization() const;
  std::optional<double> CPUFrequencyHz() const;
  std::optional<float> MemoryUtilization() const;
  std::optional<float> SystemPower() const;
  std::optional<float> InputVoltage() const;
  std::optional<float> BatteryCharge() const;
  std::optional<float> BatteryLoad() const;
  void MarkActive();

private:
  // ROS messages
  using SystemTelemetry = oasis_msgs::msg::SystemTelemetry;
  using UPSStatus = oasis_msgs::msg::UPSStatus;

  // ROS 2 subscriber callbacks
  void OnSystemTelemetry(const SystemTelemetry::SharedPtr msg);
  void OnUPSStatus(const UPSStatus::SharedPtr msg);

  // ROS parameters
  const std::string m_rosNamespace;
  rclcpp::Subscription<SystemTelemetry>::SharedPtr m_telemetrySubscriber;
  rclcpp::Subscription<UPSStatus>::SharedPtr m_upsStatusSubscriber;

  // GUI parameters
  std::chrono::time_point<std::chrono::steady_clock> m_lastActive;
  CTemperature m_cpuTemperature;
  std::optional<float> m_cpuUtilization;
  std::optional<double> m_cpuFrequencyHz;
  std::optional<float> m_memoryUtilization;
  std::optional<float> m_systemPower;
  std::optional<float> m_inputVoltage;
  bool m_hasUpsStatus{false};
  std::optional<float> m_batteryCharge;
  std::optional<float> m_batteryLoadWatts;

  // Synchronization parameters
  mutable std::mutex m_mutex;
};
} // namespace SMART_HOME
} // namespace KODI
