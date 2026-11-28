/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2.h"

#include "smarthome/guibridge/SmartHomeGuiManager.h"
#include "smarthome/ros2/Ros2Node.h"
#include "smarthome/ros2/Ros2VideoSubscription.h" //! @todo Header needed?
#include "threads/Thread.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <chrono>

#include <image_transport/image_transport.hpp>
#include <rclcpp/rclcpp.hpp>
#include <spdlog/spdlog.h>

using namespace KODI;
using namespace SMART_HOME;

CRos2::CRos2(CSmartHomeGuiManager& guiManager,
             CSmartHomeInputManager& inputManager,
             std::vector<std::string> cmdLineArgs)
  : m_guiManager(guiManager),
    m_inputManager(inputManager),
    m_cmdLineArgs(std::move(cmdLineArgs)),
    m_node(std::make_unique<CRos2Node>(m_inputManager))
{
}

CRos2::~CRos2() = default;

void CRos2::Initialize()
{
  CLog::Log(LOGDEBUG, "ROS2: Initializing ROS with args \"{}\"",
            StringUtils::Join(m_cmdLineArgs, "\", \""));

  // Convert arguments to flat array for rclcpp::init()
  std::vector<const char*> argv;
  argv.reserve(m_cmdLineArgs.size());
  for (const auto& arg : m_cmdLineArgs)
    argv.push_back(arg.c_str());

  // Initialize ROS
  rclcpp::init(static_cast<int>(argv.size()), argv.data());

  // Join the shared spdlog flush worker before ROS unloads the backend that
  // started it
  rclcpp::on_shutdown([] { spdlog::flush_every(std::chrono::seconds::zero()); });

  // Discover camera transports while initialization errors can still disable the backend.
  CLog::Log(LOGDEBUG, "ROS2: Declared image transports: {}",
            StringUtils::Join(image_transport::getDeclaredTransports(), ", "));

  // Initialize node
  m_node->Initialize();
}

void CRos2::Deinitialize()
{
  if (rclcpp::ok())
  {
    CLog::Log(LOGDEBUG, "ROS2: Deinitializing ROS");
    rclcpp::shutdown();
  }

  if (m_node)
  {
    m_node->Deinitialize();
    m_node.reset();
  }
}

std::shared_ptr<const PERIPHERALS::CPeripheral> CRos2::GetRemotePeripheral(
    const std::string& hostname, const std::string& address) const
{
  return m_node ? m_node->GetRemotePeripheral(hostname, address) : nullptr;
}

std::optional<int> CRos2::GetControllerBattery(const std::string& hostname) const
{
  return m_node ? m_node->GetControllerBattery(hostname) : std::nullopt;
}

void CRos2::RegisterImageTopic(const ImageSubscriptionKey& subscription)
{
  if (!m_node)
    return;

  CSmartHomeGuiBridge& guiBridge = m_guiManager.GetGuiBridge(subscription);
  m_node->RegisterImageTopic(guiBridge, subscription);
}

void CRos2::UnregisterImageTopic(const ImageSubscriptionKey& subscription)
{
  if (m_node)
    m_node->UnregisterImageTopic(subscription);
}

ISystemHealthHUD* CRos2::GetSystemHealthHUD() const
{
  return m_node->GetSystemHealthHUD();
}

IPowerMeterHUD* CRos2::GetPowerMeterHUD() const
{
  return m_node->GetPowerMeterHUD();
}

IVehicleHUD* CRos2::GetVehicleHUD() const
{
  return m_node->GetVehicleHUD();
}

void CRos2::FrameMove()
{
  //! @todo Remove GUI dependency
  m_node->FrameMove();
}
