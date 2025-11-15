/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2Node.h"

#include "Ros2InputPublisher.h"
#include "Ros2LabSubscriber.h"
#include "Ros2NowPlayingPublisher.h"
#include "Ros2StationSubscriber.h"
#include "Ros2SystemHealthManager.h"
#include "Ros2VideoSubscription.h"
#include "ServiceBroker.h"
#include "network/Network.h"
#include "threads/Thread.h"

#include <sstream>

#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;

namespace
{
constexpr const char* ROS_NAMESPACE = "oasis"; //! @todo

// Name of the ROS node (hostname is concatenated at the end)
constexpr const char* NODE_NAME_PREFIX = "kodi_"; //! @todo Get from version.txt

// Name of the OS thread
constexpr const char* THREAD_NAME = "ROS2"; // TODO

// Hostname to use when the system hostname can't be retrieved
constexpr const char* HOSTNAME_UNKNOWN = "unknown";
} // namespace

CRos2Node::CRos2Node(CSmartHomeInputManager& inputManager)
  : m_inputManager(inputManager),
    m_systemHealthManager(std::make_unique<CRos2SystemHealthManager>(ROS_NAMESPACE))
{
}

CRos2Node::~CRos2Node() = default;

void CRos2Node::Initialize()
{
  std::string hostname;
  if (!CServiceBroker::GetNetwork().GetHostName(hostname) || hostname.empty())
    hostname = HOSTNAME_UNKNOWN;

  // Replace "-" with "_" in hostname
  StringUtils::Replace(hostname, '-', '_');

  // Can't make virtual calls from constructor
  m_node = std::make_shared<rclcpp::Node>(NODE_NAME_PREFIX + hostname);
  m_executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  m_executor->add_node(m_node);
  m_thread = std::make_unique<CThread>(this, THREAD_NAME);

  // Managers
  m_systemHealthManager->Initialize(m_node);

  // Publishers
  m_peripheralPublisher = std::make_unique<CRos2InputPublisher>(m_node, m_inputManager, hostname);
  m_peripheralPublisher->Initialize();
  m_nowPlayingPublisher = std::make_unique<CRos2NowPlayingPublisher>(m_node, std::move(hostname));
  m_nowPlayingPublisher->Initialize();

  // Subscribers
  m_labSubscriber = std::make_unique<CRos2LabSubscriber>(m_node);
  m_labSubscriber->Initialize();
  m_stationSubscriber = std::make_unique<CRos2StationSubscriber>(m_node);
  m_stationSubscriber->Initialize();

  // Create thread
  m_thread->Create(false);
}

void CRos2Node::Deinitialize()
{
  // Deinitialize ROS
  if (m_executor)
    m_executor->cancel();

  rclcpp::shutdown();

  // Stop thread
  if (m_thread)
    m_thread->StopThread(true);

  if (m_executor && m_node)
    m_executor->remove_node(m_node);

  m_systemHealthManager->Deinitialize();

  for (const auto& videoSub : m_videoSubs)
    videoSub.second->Deinitialize();
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

  if (m_labSubscriber)
  {
    m_labSubscriber->Deinitialize();
    m_labSubscriber.reset();
  }

  if (m_stationSubscriber)
  {
    m_stationSubscriber->Deinitialize();
    m_stationSubscriber.reset();
  }

  m_thread.reset();
  m_executor.reset();
  m_node.reset();
}

namespace
{
std::string MakeSubscriptionKey(const std::string& topic, const std::string& imageTransport)
{
  return topic + "|" + imageTransport;
}
} // namespace

void CRos2Node::RegisterImageTopic(CSmartHomeGuiBridge& guiBridge,
                                   const std::string& topic,
                                   const std::string& imageTransport)
{
  const std::string subscriptionKey = MakeSubscriptionKey(topic, imageTransport);

  if (m_videoSubs.find(subscriptionKey) == m_videoSubs.end())
  {
    auto subscription =
        std::make_unique<CRos2VideoSubscription>(m_node, guiBridge, topic, imageTransport);
    subscription->Initialize();
    m_videoSubs.insert(std::make_pair(subscriptionKey, std::move(subscription)));
  }
}

void CRos2Node::UnregisterImageTopic(const std::string& topic, const std::string& imageTransport)
{
  const std::string subscriptionKey = MakeSubscriptionKey(topic, imageTransport);

  auto it = m_videoSubs.find(subscriptionKey);
  if (it != m_videoSubs.end())
    m_videoSubs.erase(it);
}

ISystemHealthHUD* CRos2Node::GetSystemHealthHUD() const
{
  return m_systemHealthManager.get();
}

ILabHUD* CRos2Node::GetLabHUD() const
{
  return m_labSubscriber.get();
}

IStationHUD* CRos2Node::GetStationHUD() const
{
  return m_stationSubscriber.get();
}

void CRos2Node::FrameMove()
{
  for (const auto& videoSub : m_videoSubs)
    videoSub.second->FrameMove();
}

void CRos2Node::Run()
{
  // Enter ROS main loop
  if (m_executor)
    m_executor->spin();
}
