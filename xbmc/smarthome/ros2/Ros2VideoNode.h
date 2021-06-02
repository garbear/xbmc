/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/ros2/Ros2Node.h"
#include "smarthome/streams/ISmartHomeStream.h" // TODO: Move to cpp

#include <memory>

#include <image_transport/image_transport.hpp>
#include <image_transport/subscriber.hpp>
#include <sensor_msgs/msg/image.hpp>

struct SwsContext;

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
class CSmartHomeStreams;
class ISmartHomeStream;

class CRos2VideoNode : public CRos2Node
{
public:
  CRos2VideoNode(CSmartHomeStreams& streams);
  ~CRos2VideoNode() override = default;

protected:
  // Implementation of CRos2Node
  const char* NodeName() const override;
  const char* ThreadName() const override;
  void InitializeInternal(std::shared_ptr<rclcpp::Node> node) override;
  void DeinitializeInternal() override;

private:
  // ROS callback
  void ReceiveImage(const std::shared_ptr<const sensor_msgs::msg::Image>& msg);

  // Construction parameters
  CSmartHomeStreams& m_streams;

  // Initialization parameters
  std::shared_ptr<rclcpp::Node> m_node;

  // ROS parameters
  std::unique_ptr<image_transport::ImageTransport> m_imgTransport;
  std::unique_ptr<image_transport::Subscriber> m_imgSubscriber;

  // Smart home parameters
  std::unique_ptr<ISmartHomeStream> m_stream;

  // Video parameters
  SwsContext* m_pixelScaler = nullptr;
};
} // namespace SMART_HOME
} // namespace KODI
