/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CAppParamParser;

namespace KODI
{

namespace SMART_HOME
{
class CRos2;
class CSmartHomeGuiBridge;

class CSmartHomeServices
{
public:
  CSmartHomeServices(const CAppParamParser& params);
  ~CSmartHomeServices();

  void Initialize();
  void Deinitialize();

  // Smart home subsystems (const)
  const CSmartHomeGuiBridge& GuiBridge() const { return *m_guiBridge; }
  const CRos2& Ros2() const { return *m_ros2; }

  // Smart home subsystems (mutable)
  CSmartHomeGuiBridge& GuiBridge() { return *m_guiBridge; }
  CRos2& Ros2() { return *m_ros2; }

  //! @todo Remove GUI dependency
  void FrameMove();

private:
  // Subsystems
  std::unique_ptr<CSmartHomeGuiBridge> m_guiBridge;
  std::unique_ptr<CRos2> m_ros2;
};

} // namespace SMART_HOME
} // namespace KODI
