/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/IRunnable.h"
#include "threads/Thread.h" // TODO: Move to cpp

#include <memory>

class CThread;

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
class CRos2Node : public IRunnable
{
public:
  CRos2Node() = default;
  virtual ~CRos2Node();

  void Initialize();
  void Deinitialize();

  //! @todo Remove GUI dependency
  virtual void FrameMove() {}

protected:
  /*!
   * \brief Name of the ROS node in the ROS computation graph
   */
  virtual const char* NodeName() const = 0;

  /*!
   * \brief Name of the OS thread
   */
  virtual const char* ThreadName() const = 0;

  /*!
   * \brief Initialize the node
   *
   * \param node The ROS node handle
   */
  virtual void InitializeInternal(std::shared_ptr<rclcpp::Node> node) = 0;

  /*!
   * \brief Deinitialize the node
   */
  virtual void DeinitializeInternal() = 0;

private:
  // Implementation of IRunnable
  void Run() override;

  // ROS parameters
  std::shared_ptr<rclcpp::Node> m_node;

  // Threading parameters
  std::unique_ptr<CThread> m_thread;
};
} // namespace SMART_HOME
} // namespace KODI
