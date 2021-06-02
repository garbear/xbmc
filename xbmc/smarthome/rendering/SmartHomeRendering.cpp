/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeRendering.h"

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/streams/RPStreamManager.h"
#include "smarthome/guibridge/GUIRenderTargetFactory.h"
#include "smarthome/guibridge/SmartHomeGuiBridge.h"
#include "smarthome/streams/SmartHomeStreams.h"
#include "utils/log.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeRendering::CSmartHomeRendering(CSmartHomeGuiBridge& guiBridge, CSmartHomeStreams& streams)
  : m_guiBridge(guiBridge), m_streams(streams)
{
}

CSmartHomeRendering::~CSmartHomeRendering() = default;

void CSmartHomeRendering::Initialize()
{
  // Initialize process info
  m_processInfo.reset(RETRO::CRPProcessInfo::CreateInstance());
  if (!m_processInfo)
  {
    CLog::Log(LOGERROR, "SMARTHOME: Failed to create - no process info registered");
  }
  else
  {
    // Initialize render manager
    m_renderManager = std::make_unique<RETRO::CRPRenderManager>(*m_processInfo);

    // Initialize stream subsystem
    m_streamManager = std::make_unique<RETRO::CRPStreamManager>(*m_renderManager, *m_processInfo);
    m_streams.Initialize(*m_streamManager);

    // Initialize GUI
    m_renderTargetFactory = std::make_unique<CGUIRenderTargetFactory>(m_renderManager.get());
    m_guiBridge.RegisterRenderer(m_renderTargetFactory.get());

    CLog::Log(LOGDEBUG, "SMARTHOME: Created renderer");
  }
}

void CSmartHomeRendering::Deinitialize()
{
  // Deinitialize GUI
  m_guiBridge.UnregisterRenderer();
  m_renderTargetFactory.reset();

  // Deinitialize stream subsystem
  m_streams.Deinitialize();
  m_streamManager.reset();

  // Deinitialize render manager
  m_renderManager.reset();

  // Deinitialize process info
  m_processInfo.reset();
}

void CSmartHomeRendering::FrameMove()
{
  if (m_renderManager)
    m_renderManager->FrameMove();
}
