/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace KODI
{
namespace RETRO
{
class CGUIGameControl;
class CRPProcessInfo;
class CRPRenderManager;
class CRPStreamManager;
} // namespace RETRO

namespace SMART_HOME
{
class CGUIRenderHandle;
class CGUIRenderTargetFactory;
class CRos2;
class CSmartHomeGuiBridge;
class CSmartHomeStreams;

class CSmartHomeRendering
{
public:
  CSmartHomeRendering(CSmartHomeGuiBridge& guiBridge, CSmartHomeStreams& streams);
  ~CSmartHomeRendering();

  void Initialize();
  void Deinitialize();

  // GUI functions
  void FrameMove();

private:
  // Subsystems
  CSmartHomeGuiBridge& m_guiBridge;
  CSmartHomeStreams& m_streams;

  // GUI parameters
  std::unique_ptr<CGUIRenderTargetFactory> m_renderTargetFactory;

  // RetroPlayer parameters
  std::unique_ptr<RETRO::CRPProcessInfo> m_processInfo;
  std::unique_ptr<RETRO::CRPRenderManager> m_renderManager;
  std::unique_ptr<RETRO::CRPStreamManager> m_streamManager;
};

} // namespace SMART_HOME
} // namespace KODI
