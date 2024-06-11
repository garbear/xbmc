
/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2SystemHealthSubscriber.h"

#include "utils/log.h"

#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;
using namespace std::literals::chrono_literals;
using std::placeholders::_1;

namespace
{
constexpr const char* SUBSCRIBE_TELEMETRY_TOPIC = "system_telemetry";

constexpr std::chrono::seconds ACTIVE_TIMEOUT_SECS = 10s;
} // namespace

CRos2SystemHealthSubscriber::CRos2SystemHealthSubscriber(std::string rosNamespace)
  : m_rosNamespace(std::move(rosNamespace))
{
}

void CRos2SystemHealthSubscriber::Initialize(std::shared_ptr<rclcpp::Node> node,
                                             const std::string& systemName)
{
  if (m_telemetrySubscriber)
  {
    // Already initialized
    return;
  }

  // Calculate topic name
  const std::string subscribeTelemetryTopic =
      std::string("/") + m_rosNamespace + "/" + systemName + "/" + SUBSCRIBE_TELEMETRY_TOPIC;

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeTelemetryTopic);

  // QoS policy
  rclcpp::SensorDataQoS qos;
  const size_t queueSize = 10;
  qos.keep_last(queueSize);

  // Subscribers
  m_telemetrySubscriber = node->create_subscription<SystemTelemetry>(
      subscribeTelemetryTopic, qos,
      std::bind(&CRos2SystemHealthSubscriber::OnSystemTelemetry, this, _1));
}

void CRos2SystemHealthSubscriber::Deinitialize()
{
  m_telemetrySubscriber.reset();
}

bool CRos2SystemHealthSubscriber::IsActive() const
{
  bool isActive = false;

  std::lock_guard<std::mutex> lock(m_mutex);

  // Check if m_lastActive is valid
  if (m_lastActive.time_since_epoch().count() > 0)
  {
    // Get current time
    const auto now = std::chrono::steady_clock::now();

    // Check if the system is active
    isActive = (now - m_lastActive) < ACTIVE_TIMEOUT_SECS;
  }

  return isActive;
}

CTemperature CRos2SystemHealthSubscriber::CPUTemperature() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_cpuTemperature;
}

float CRos2SystemHealthSubscriber::CPUUtilization() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_cpuUtilization;
}

void CRos2SystemHealthSubscriber::OnSystemTelemetry(const SystemTelemetry::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Update the last active time
  m_lastActive = std::chrono::steady_clock::now();

  // Update system health parameters
  if (msg->cpu_temperature != 0.0)
     m_cpuTemperature= CTemperature::CreateFromCelsius(msg->cpu_temperature);
  m_cpuUtilization = msg->cpu_utilization;
}
