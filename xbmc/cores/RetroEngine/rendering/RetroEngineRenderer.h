/*
 *  Copyright (C) 2021-2023 Team Kodi
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
class CRPProcessInfo;
class CRPRenderManager;
class CRPStreamManager;
} // namespace RETRO

namespace RETRO_ENGINE
{
class CGUIRenderTargetFactory;
class CRetroEngineGuiBridge;
class CRetroEngineStreamManager;

class CRetroEngineRenderer
{
public:
  CRetroEngineRenderer(CRetroEngineGuiBridge& guiBridge, CRetroEngineStreamManager& streamManager);
  ~CRetroEngineRenderer();

  void Initialize();
  void Deinitialize();

  // GUI functions
  void FrameMove();

private:
  // Subsystems
  CRetroEngineGuiBridge& m_guiBridge;
  CRetroEngineStreamManager& m_streamManager;

  // GUI parameters
  std::unique_ptr<CGUIRenderTargetFactory> m_renderTargetFactory;

  // RetroPlayer parameters
  std::unique_ptr<RETRO::CRPProcessInfo> m_processInfo;
  std::unique_ptr<RETRO::CRPRenderManager> m_renderManager;
  std::unique_ptr<RETRO::CRPStreamManager> m_retroStreamManager;
};

} // namespace RETRO_ENGINE
} // namespace KODI
