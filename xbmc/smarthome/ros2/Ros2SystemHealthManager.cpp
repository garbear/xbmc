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

bool CRos2SystemHealthManager::IsActive(const std::string& systemName)
{
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  return it->second.IsActive();
}

CTemperature CRos2SystemHealthManager::CPUTemperature(const std::string& systemName)
{
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  return it->second.CPUTemperature();
}

float CRos2SystemHealthManager::CPUUtilization(const std::string& systemName)
{
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  return it->second.CPUUtilization();
}

void CRos2SystemHealthManager::AddSystem(const std::string& systemName)
{
  if (!m_node)
    return;

  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    // Add a new subscriber to the map
    m_systemHealths.emplace(systemName, m_rosNamespace);

    // Initialize subscriber
    m_systemHealths.at(systemName).Initialize(m_node, systemName);
  }
}
