/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <mutex>

#include <oasis_msgs/msg/system_telemetry.hpp>
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
  CRos2SystemHealthSubscriber(CRos2SystemHealthSubscriber&& rhs);

  // ROS functions
  void Initialize(std::shared_ptr<rclcpp::Node> node, const std::string& systemName);
  void Deinitialize();

  // GUI functions
  bool IsActive() const;
  float TemperatureDegC() const;

private:
  // ROS messages
  using SystemTelemetry = oasis_msgs::msg::SystemTelemetry;

  // ROS 2 subscriber callbacks
  void OnSystemTelemetry(const SystemTelemetry::SharedPtr msg);

  // ROS parameters
  const std::string m_rosNamespace;
  rclcpp::Subscription<SystemTelemetry>::SharedPtr m_telemetrySubscriber;

  // GUI parameters
  std::chrono::time_point<std::chrono::steady_clock> m_lastActive;
  float m_temperatureDegC{};

  // Synchronization parameters
  mutable std::mutex m_mutex;
};
} // namespace SMART_HOME
} // namespace KODI
