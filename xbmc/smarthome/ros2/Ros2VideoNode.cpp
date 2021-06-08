/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2VideoNode.h"

#include "smarthome/ros2/Ros2VideoSubscription.h"
#include "smarthome/streams/SmartHomeStreamManager.h"
#include "smarthome/streams/SmartHomeStreamSwFramebuffer.h"
#include "utils/log.h"

#include <cstring>

#include <image_transport/transport_hints.hpp>
#include <rclcpp/rclcpp.hpp>

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

using namespace KODI;
using namespace SMART_HOME;

namespace
{

constexpr const char* ROS_NAMESPACE = "oasis"; // TODO

// Name of the ROS node
constexpr const char* NODE_NAME = "kodi_lenovo"; // TODO: Hostname?

constexpr const char* MACHINE_NAME = "netbook";

constexpr const char* IMAGE_TOPIC = "foreground";

// Name of the OS thread
constexpr const char* THREAD_NAME = "ROS2"; // TODO

} // namespace

CRos2VideoNode::CRos2VideoNode(CSmartHomeGuiBridge& guiBridge) : m_guiBridge(guiBridge)
{
}

CRos2VideoNode::~CRos2VideoNode() = default;

const char* CRos2VideoNode::NodeName() const
{
  return NODE_NAME;
}

const char* CRos2VideoNode::ThreadName() const
{
  return THREAD_NAME;
}

void CRos2VideoNode::InitializeInternal(std::shared_ptr<rclcpp::Node> node)
{
  m_node = std::move(node);
  m_imgTransport = std::make_unique<image_transport::ImageTransport>(m_node);

  //! @todo Multiple GUI bridges are needed to handle multiple feeds
  //const std::vector<std::string> machines = {"netbook", "lenovo"};
  //const std::vector<std::string> topics = {"image_raw", "foreground"};
  const std::vector<std::string> machines = {MACHINE_NAME};
  const std::vector<std::string> topics = {IMAGE_TOPIC};
  for (const auto& machine : machines)
  {
    for (const auto& topic : topics)
    {
      auto subscription = std::make_unique<CRos2VideoSubscription>(m_guiBridge, ROS_NAMESPACE, machine, topic);
      subscription->Initialize(*m_node, *m_imgTransport);
      m_subscriptions.emplace_back(std::move(subscription));
    }
  }
}

void CRos2VideoNode::DeinitializeInternal()
{
  for (auto& subscription : m_subscriptions)
    subscription->Deinitialize();
  m_subscriptions.clear();

  m_imgTransport.reset();
  m_node.reset();
}

void CRos2VideoNode::FrameMove()
{
  //! @todo Remove GUI dependency
  for (const auto& subscription : m_subscriptions)
    subscription->FrameMove();
}
