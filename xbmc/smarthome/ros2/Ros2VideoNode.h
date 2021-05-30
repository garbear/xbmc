/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/ros2/Ros2Node.h"
#include "smarthome/ros2/Ros2VideoSubscription.h" //! @todo Move to cpp

#include <memory>
#include <vector>

#include <image_transport/image_transport.hpp>

struct SwsContext;

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
class CRos2VideoSubscription;
class CSmartHomeGuiBridge;

class CRos2VideoNode : public CRos2Node
{
public:
  CRos2VideoNode(CSmartHomeGuiBridge& guiBridge);
  ~CRos2VideoNode() override;

  //! @todo Remove GUI dependency
  void FrameMove() override;

protected:
  // Implementation of CRos2Node
  const char* NodeName() const override;
  const char* ThreadName() const override;
  void InitializeInternal(std::shared_ptr<rclcpp::Node> node) override;
  void DeinitializeInternal() override;

private:
  // Construction parameters
  CSmartHomeGuiBridge& m_guiBridge;

  // Initialization parameters
  std::shared_ptr<rclcpp::Node> m_node;

  // ROS parameters
  std::unique_ptr<image_transport::ImageTransport> m_imgTransport;
  std::vector<std::unique_ptr<CRos2VideoSubscription>> m_subscriptions;
};
} // namespace SMART_HOME
} // namespace KODI
