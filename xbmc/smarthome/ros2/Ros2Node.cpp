/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2Node.h"

#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;

CRos2Node::~CRos2Node() = default;

void CRos2Node::Initialize()
{
  // Can't make virtual calls from constructor
  m_node = std::make_shared<rclcpp::Node>(NodeName());
  m_thread = std::make_unique<CThread>(this, ThreadName());

  // Initialize subclass
  InitializeInternal(m_node);

  // Create thread
  m_thread->Create(false);
}

void CRos2Node::Deinitialize()
{
  // Signal thread
  m_thread->StopThread(false);

  // Deinitialize subclass
  DeinitializeInternal();

  // Stop thread
  m_thread->StopThread(true);

  m_thread.reset();
  m_node.reset();
}

void CRos2Node::Run()
{
  rclcpp::spin(m_node);
}
