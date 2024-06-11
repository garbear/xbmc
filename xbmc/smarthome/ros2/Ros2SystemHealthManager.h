/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/ISystemHealthHUD.h"

#include <map>
#include <memory>
#include <string>

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
class CRos2SystemHealthSubscriber;

/*!
 * \brief ROS 2 subscription manager for system health
 */
class CRos2SystemHealthManager : public ISystemHealthHUD
{
public:
  CRos2SystemHealthManager(std::string rosNamespace);
  ~CRos2SystemHealthManager();

  // ROS functions
  void Initialize(std::shared_ptr<rclcpp::Node> node);
  void Deinitialize();
  void AddSystem(const std::string& systemName);

  // Implementation of ISystemHealthHUD
  bool IsActive(const std::string& systemName) const override;
  float TemperatureDegC(const std::string& systemName) const override;

private:
  // ROS parameters
  const std::string m_rosNamespace;
  std::shared_ptr<rclcpp::Node> m_node;

  // System health parameters
  std::map<std::string, CRos2SystemHealthSubscriber> m_systemHealths;
};
} // namespace SMART_HOME
} // namespace KODI
