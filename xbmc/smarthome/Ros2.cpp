/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2.h"

#include "smarthome/Ros2VideoNode.h"
#include "threads/Thread.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;

CRos2::CRos2(std::vector<std::string> cmdLineArgs, CSmartHomeStreams& streams)
  : m_cmdLineArgs(std::move(cmdLineArgs)), m_streams(streams)
{
}

CRos2::~CRos2() = default;

void CRos2::Initialize()
{
  CLog::Log(LOGDEBUG, "ROS2: Initializing ROS with args \"{}\"",
            StringUtils::Join(m_cmdLineArgs, "\", \""));

  // Convert arguments to flat array for rclcpp::init()
  int argc = 0;
  char const* const* argv = nullptr;
  TranslateArguments(m_cmdLineArgs, argc, argv);

  // Initialize ROS
  rclcpp::init(argc, argv);

  // Create nodes
  std::unique_ptr<CRos2Node> videoNode = std::make_unique<CRos2VideoNode>(m_streams);
  videoNode->Initialize();
  m_nodes.emplace_back(std::move(videoNode));
}

void CRos2::Deinitialize()
{
  // Destroy nodes
  for (const auto& node : m_nodes)
    node->Deinitialize();
  m_nodes.clear();

  // Deinitialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Deinitializing ROS");
  rclcpp::shutdown();
}

void CRos2::TranslateArguments(const std::vector<std::string>& cmdLineArgs,
                               int& argc,
                               char const* const*& argv)
{
  std::vector<const char*> tempCmdLineArgs;
  tempCmdLineArgs.reserve(cmdLineArgs.size());

  unsigned int i = 0;
  for (const auto& arg : cmdLineArgs)
    tempCmdLineArgs[i++] = arg.c_str();

  argc = static_cast<int>(cmdLineArgs.size());
  argv = argc > 0 ? reinterpret_cast<char const* const*>(cmdLineArgs.data()) : nullptr;
}
