/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroEngineRenderer.h"

#include "cores/RetroEngine/guibridge/GUIRenderTargetFactory.h"
#include "cores/RetroEngine/guibridge/RetroEngineGuiBridge.h"
#include "cores/RetroEngine/streams/RetroEngineStreamManager.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/streams/RPStreamManager.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CRetroEngineRenderer::CRetroEngineRenderer(CRetroEngineGuiBridge& guiBridge,
                                           CRetroEngineStreamManager& streamManager)
  : m_guiBridge(guiBridge), m_streamManager(streamManager)
{
}

CRetroEngineRenderer::~CRetroEngineRenderer() = default;

void CRetroEngineRenderer::Initialize()
{
  // Initialize process info
  m_processInfo = RETRO::CRPProcessInfo::CreateInstance();
  if (!m_processInfo)
  {
    CLog::Log(LOGERROR, "RETROENGINE: Failed to create - no process info registered");
  }
  else
  {
    // Initialize render manager
    m_renderManager = std::make_unique<RETRO::CRPRenderManager>(*m_processInfo);

    // Initialize stream subsystem
    m_retroStreamManager =
        std::make_unique<RETRO::CRPStreamManager>(*m_renderManager, *m_processInfo);
    m_streamManager.Initialize(*m_retroStreamManager);

    // Initialize GUI
    m_renderTargetFactory = std::make_unique<CGUIRenderTargetFactory>(*m_renderManager);
    m_guiBridge.RegisterRenderer(*m_renderTargetFactory);

    CLog::Log(LOGDEBUG, "RETROENGINE: Created renderer");
  }
}

void CRetroEngineRenderer::Deinitialize()
{
  // Deinitialize GUI
  m_guiBridge.UnregisterRenderer();
  m_renderTargetFactory.reset();

  // Deinitialize stream subsystem
  m_streamManager.Deinitialize();
  m_retroStreamManager.reset();

  // Deinitialize render manager
  m_renderManager.reset();

  // Deinitialize process info
  m_processInfo.reset();
}

void CRetroEngineRenderer::FrameMove()
{
  if (m_renderManager)
    m_renderManager->FrameMove();
}
