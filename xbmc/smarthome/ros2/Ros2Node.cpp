/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2Node.h"

#include "Ros2InputPublisher.h"
#include "Ros2NowPlayingPublisher.h"
#include "Ros2PeripheralSubscriber.h"
#include "Ros2PowerMeterManager.h"
#include "Ros2SystemHealthManager.h"
#include "Ros2VehicleManager.h"
#include "Ros2VideoSubscription.h"
#include "ServiceBroker.h"
#include "network/Network.h"
#include "peripherals/devices/Peripheral.h"
#include "platform/Environment.h"
#include "smarthome/input/SmartHomeInputManager.h"
#include "smarthome/utils/RosNameUtils.h"
#include "threads/Thread.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <set>
#include <sstream>
#include <string_view>

#include <oasis_msgs/msg/peripheral_scan.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;

namespace
{
// Name of the ROS node (hostname is concatenated at the end)
constexpr const char* NODE_NAME_PREFIX = "kodi_"; //! @todo Get from version.txt

// Name of the OS thread
constexpr const char* THREAD_NAME = "ROS2"; // TODO

// Hostname to use when the system hostname can't be retrieved
constexpr const char* HOSTNAME_UNKNOWN = "unknown";

using PeripheralScan = oasis_msgs::msg::PeripheralScan;
constexpr std::string_view PERIPHERAL_SCAN_TYPE = rosidl_generator_traits::name<PeripheralScan>();
} // namespace

CRos2Node::CRos2Node(CSmartHomeInputManager& inputManager)
  : m_inputManager(inputManager),
    m_rosNamespace(NormalizeRosNamespace(CEnvironment::getenv("ROS_NAMESPACE"))),
    m_systemHealthManager(std::make_unique<CRos2SystemHealthManager>(m_rosNamespace)),
    m_powerMeterManager(std::make_unique<CRos2PowerMeterManager>(
        m_rosNamespace,
        [this](const std::string& systemName) { m_systemHealthManager->MarkActive(systemName); })),
    m_vehicleManager(std::make_unique<CRos2VehicleManager>(m_rosNamespace))
{
}

CRos2Node::~CRos2Node() = default;

void CRos2Node::Initialize()
{
  std::string hostname;
  if (!CServiceBroker::GetNetwork().GetHostName(hostname) || hostname.empty())
    hostname = HOSTNAME_UNKNOWN;

  m_localHostname = NormalizeRosHostname(std::move(hostname));

  // Can't make virtual calls from constructor
  m_node = std::make_shared<rclcpp::Node>(NODE_NAME_PREFIX + m_localHostname, m_rosNamespace);
  m_executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  m_executor->add_node(m_node);
  m_thread = std::make_unique<CThread>(this, THREAD_NAME);

  // Managers
  m_systemHealthManager->Initialize(m_node);
  m_powerMeterManager->Initialize(m_node);
  m_vehicleManager->Initialize(m_node);

  // Publishers
  m_peripheralPublisher =
      std::make_unique<CRos2InputPublisher>(m_node, m_inputManager, m_localHostname, m_rosNamespace);
  m_peripheralPublisher->Initialize();
  m_localPeripheralTopic = m_node->get_node_topics_interface()->resolve_topic_name(
      BuildRosName(m_rosNamespace, {m_localHostname, PERIPHERALS_TOPIC}));
  DiscoverPeripheralPublishers();
  m_discoveryTimer = m_node->create_wall_timer(std::chrono::seconds(10),
                                               [this] { DiscoverPeripheralPublishers(); });

  m_nowPlayingPublisher =
      std::make_unique<CRos2NowPlayingPublisher>(m_node, m_localHostname, m_rosNamespace);
  m_nowPlayingPublisher->Initialize();

  // Create thread
  m_thread->Create(false);
}

void CRos2Node::Deinitialize()
{
  // Stop thread
  if (m_thread)
    m_thread->StopThread(true);

  m_discoveryTimer.reset();
  {
    std::lock_guard lock(m_peripheralMutex);
    m_peripheralSubs.clear();
  }
  m_powerMeterManager->Deinitialize();
  m_systemHealthManager->Deinitialize();

  for (const auto& videoSub : m_videoSubs)
    videoSub.second.subscription->Deinitialize();
  m_videoSubs.clear();

  if (m_nowPlayingPublisher)
  {
    m_nowPlayingPublisher->Deinitialize();
    m_nowPlayingPublisher.reset();
  }

  if (m_peripheralPublisher)
  {
    m_peripheralPublisher->Deinitialize();
    m_peripheralPublisher.reset();
  }

  m_thread.reset();
  m_executor.reset();
  m_node.reset();
}

void CRos2Node::DiscoverPeripheralPublishers()
{
  const std::string prefix = m_rosNamespace == "/" ? "/" : m_rosNamespace + "/";

  try
  {
    std::lock_guard lock(m_peripheralMutex);
    std::set<std::string> hosts;
    for (const auto& [topic, types] : m_node->get_topic_names_and_types())
    {
      if (topic == m_localPeripheralTopic || !topic.starts_with(prefix) ||
          std::ranges::find(types, PERIPHERAL_SCAN_TYPE) == types.end())
        continue;

      const auto separator = topic.find('/', prefix.size());
      if (separator == std::string::npos || separator == prefix.size() ||
          std::string_view(topic).substr(separator + 1) != PERIPHERALS_TOPIC ||
          m_node->get_node_topics_interface()->resolve_topic_name(topic) != topic)
        continue;

      // Our own subscription keeps the topic in the graph after the publisher leaves.
      const auto publishers = m_node->get_publishers_info_by_topic(topic);
      if (std::ranges::none_of(publishers, [](const auto& p)
                               { return p.topic_type() == PERIPHERAL_SCAN_TYPE; }))
        continue;

      const auto host = topic.substr(prefix.size(), separator - prefix.size());
      hosts.insert(host);
      if (!m_peripheralSubs.contains(host))
      {
        auto devices =
            std::make_shared<CRos2PeripheralSubscriber>(CServiceBroker::GetPeripherals());
        auto subscription = m_node->create_subscription<PeripheralScan>(
            topic, 1,
            [receiver = std::weak_ptr(devices)](const PeripheralScan& scan)
            {
              if (const auto devices = receiver.lock())
                devices->Update(scan.peripherals);
            });
        m_peripheralSubs.emplace(host, PeripheralSubscription{devices, std::move(subscription)});
      }
    }
    std::erase_if(m_peripheralSubs,
                  [&hosts](const auto& entry) { return !hosts.contains(entry.first); });
  }
  catch (const std::exception& e)
  {
    if (rclcpp::ok())
      CLog::Log(LOGERROR, "ROS2: Peripheral discovery failed: {}", e.what());
  }
}

std::shared_ptr<const PERIPHERALS::CPeripheral> CRos2Node::GetRemotePeripheral(
    const std::string& hostname, const std::string& address) const
{
  std::lock_guard lock(m_peripheralMutex);
  const auto it = m_peripheralSubs.find(hostname);
  return it != m_peripheralSubs.end() ? it->second.devices->GetPeripheral(address) : nullptr;
}

std::optional<int> CRos2Node::GetControllerBattery(const std::string& hostname) const
{
  if (hostname == m_localHostname)
  {
    for (const auto& peripheral : m_inputManager.GetPeripherals())
    {
      if (peripheral->Type() == PERIPHERALS::PERIPHERAL_JOYSTICK)
      {
        const int level = peripheral->GetBattery().level;
        if (level >= 0 && level <= 100)
          return level;
      }
    }
    return std::nullopt;
  }

  std::lock_guard lock(m_peripheralMutex);
  const auto it = m_peripheralSubs.find(hostname);
  return it != m_peripheralSubs.end() ? it->second.devices->GetControllerBattery()
                                      : std::nullopt;
}

void CRos2Node::RegisterImageTopic(CSmartHomeGuiBridge& guiBridge,
                                   const ImageSubscriptionKey& subscription)
{
  auto it = m_videoSubs.find(subscription);
  if (it != m_videoSubs.end())
  {
    ++it->second.referenceCount;
    return;
  }

  auto videoSubscription = std::make_unique<CRos2VideoSubscription>(
      m_node, guiBridge, subscription.first, subscription.second);
  videoSubscription->Initialize();
  m_videoSubs.emplace(subscription, VideoSubscription{std::move(videoSubscription), 1});
}

void CRos2Node::UnregisterImageTopic(const ImageSubscriptionKey& subscription)
{
  auto it = m_videoSubs.find(subscription);
  if (it == m_videoSubs.end())
    return;

  if (--it->second.referenceCount == 0)
  {
    it->second.subscription->Deinitialize();
    m_videoSubs.erase(it);
  }
}

ISystemHealthHUD* CRos2Node::GetSystemHealthHUD() const
{
  return m_systemHealthManager.get();
}

IPowerMeterHUD* CRos2Node::GetPowerMeterHUD() const
{
  return m_powerMeterManager.get();
}

IVehicleHUD* CRos2Node::GetVehicleHUD() const
{
  return m_vehicleManager.get();
}

void CRos2Node::FrameMove()
{
  for (const auto& videoSub : m_videoSubs)
    videoSub.second.subscription->FrameMove();
}

void CRos2Node::Run()
{
  // Enter ROS main loop
  if (m_executor)
    m_executor->spin();
}
