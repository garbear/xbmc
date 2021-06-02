/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

namespace KODI
{
namespace SMART_HOME
{

class CRos2Node;
class CSmartHomeStreams;

class CRos2
{
public:
  CRos2(CSmartHomeStreams& streams, std::vector<std::string> cmdLineArgs);
  ~CRos2();

  /*!
   * \brief Initialize ROS 2
   */
  void Initialize();

  /*!
   * \brief Deinitialize ROS 2
   */
  void Deinitialize();

private:
  // Utility functions
  // TODO: Test cases
  void TranslateArguments(const std::vector<std::string>& cmdLineArgs,
                          int& argc,
                          char const* const*& argv);

  // Construction parameters
  CSmartHomeStreams& m_streams;
  std::vector<std::string> m_cmdLineArgs;

  // ROS parameters
  std::vector<std::unique_ptr<CRos2Node>> m_nodes;
};
} // namespace SMART_HOME
} // namespace KODI
