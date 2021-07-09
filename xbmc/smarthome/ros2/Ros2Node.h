/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/IRunnable.h"

#include <map>
#include <memory>
#include <string>

class CThread;

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

class CRos2Node : public IRunnable
{
public:
  CRos2Node();
  virtual ~CRos2Node();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  void RegisterImageTopic(CSmartHomeGuiBridge& guiBridge, const std::string& topic);
  void UnregisterImageTopic(const std::string& topic);

  //! @todo Remove GUI dependency
  void FrameMove();

private:
  // Implementation of IRunnable
  void Run() override;

  // ROS parameters
  std::shared_ptr<rclcpp::Node> m_node;
  std::map<std::string, std::unique_ptr<CRos2VideoSubscription>> m_videoSubs; // Topic -> subscriber

  // Threading parameters
  std::unique_ptr<CThread> m_thread;
};
} // namespace SMART_HOME
} // namespace KODI
