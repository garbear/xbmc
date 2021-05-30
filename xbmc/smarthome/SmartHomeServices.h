/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SmartHomeSubsystem.h"

#include <memory>

class CAppParamParser;

namespace KODI
{
namespace RETRO
{
class CGUIGameControl;
class CGUIGameRenderManager;
class CGUIRenderHandle;
class CRPProcessInfo;
class CRPRenderManager;
class CRPStreamManager;
} // namespace RETRO

namespace SMART_HOME
{
class CRos2;

class CSmartHomeServices
{
public:
  CSmartHomeServices(const CAppParamParser& params);
  ~CSmartHomeServices();

  void Initialize();
  void Deinitialize();

  // Smart home subsystems (const)
  const CSmartHomeStreams& Streams() const { return *m_subsystems.Streams; }

  // Smart home subsystems (mutable)
  CSmartHomeStreams& Streams() { return *m_subsystems.Streams; }

  // TODO: Move to gui component
  void FrameMove();
  std::shared_ptr<RETRO::CGUIRenderHandle> RegisterControl(RETRO::CGUIGameControl& control);

private:
  // Smart home subsystems
  SmartHomeSubsystems m_subsystems;

  // Services
  std::unique_ptr<CRos2> m_ros2;

  // RetroPlayer parameters
  std::unique_ptr<RETRO::CRPProcessInfo> m_processInfo;
  std::unique_ptr<RETRO::CRPRenderManager> m_renderManager;
  std::unique_ptr<RETRO::CRPStreamManager> m_streamManager;
  std::unique_ptr<RETRO::CGUIGameRenderManager> m_gameRenderManager;
};

} // namespace SMART_HOME
} // namespace KODI
