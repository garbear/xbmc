/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SMART_HOME
{

class CRos2Node;
class CSmartHomeGuiManager;

class CRos2
{
public:
  CRos2(CSmartHomeGuiManager& guiManager, std::vector<std::string> cmdLineArgs);
  ~CRos2();

  /*!
   * \brief Initialize ROS 2 lifecycle
   */
  void Initialize();

  /*!
   * \brief Deinitialize ROS 2 lifecycle
   */
  void Deinitialize();

  void RegisterImageTopic(const std::string& topic);
  void UnregisterImageTopic(const std::string& topic);

  //! @todo Remove GUI dependency
  void FrameMove();

private:
  // Utility functions
  // TODO: Test cases
  void TranslateArguments(const std::vector<std::string>& cmdLineArgs,
                          int& argc,
                          char const* const*& argv);

  // Construction parameters
  CSmartHomeGuiManager& m_guiManager;
  std::vector<std::string> m_cmdLineArgs;

  // ROS parameters
  std::unique_ptr<CRos2Node> m_node;
};
} // namespace SMART_HOME
} // namespace KODI
