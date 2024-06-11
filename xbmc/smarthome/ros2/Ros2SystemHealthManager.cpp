/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2SystemHealthManager.h"

#include "Ros2SystemHealthSubscriber.h"

using namespace KODI;
using namespace SMART_HOME;

CRos2SystemHealthManager::CRos2SystemHealthManager(std::string rosNamespace)
  : m_rosNamespace(std::move(rosNamespace))
{
}

CRos2SystemHealthManager::~CRos2SystemHealthManager() = default;

void CRos2SystemHealthManager::Initialize(std::shared_ptr<rclcpp::Node> node)
{
  m_node = std::move(node);
}

void CRos2SystemHealthManager::Deinitialize()
{
  for (auto& systemHealth : m_systemHealths)
    systemHealth.second.Deinitialize();

  m_systemHealths.clear();
}

void CRos2SystemHealthManager::AddSystem(const std::string& systemName)
{
  if (!m_node)
    return;

  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    CRos2SystemHealthSubscriber subscriber(m_rosNamespace);

    // Initialize subscriber
    subscriber.Initialize(m_node, systemName);

    // Add the subscriber to the map
    m_systemHealths.emplace(systemName, std::move(subscriber));
  }
}

bool CRos2SystemHealthManager::IsActive(const std::string& systemName) const
{
  bool isActive = false;

  auto it = m_systemHealths.find(systemName);
  if (it != m_systemHealths.end())
    isActive = it->second.IsActive();

  return isActive;
}

float CRos2SystemHealthManager::TemperatureDegC(const std::string& systemName) const
{
  float temperatureDegC = 0.0f;

  auto it = m_systemHealths.find(systemName);
  if (it != m_systemHealths.end())
    temperatureDegC = it->second.TemperatureDegC();

  return temperatureDegC;
}
