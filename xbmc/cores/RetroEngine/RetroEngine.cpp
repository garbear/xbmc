/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroEngine.h"

#include "cores/RetroEngine/guibridge/RetroEngineGuiBridge.h"
#include "cores/RetroEngine/rendering/RetroEngineRenderer.h"
#include "cores/RetroEngine/streams/IRetroEngineStream.h"
#include "cores/RetroEngine/streams/RetroEngineStreamManager.h"
#include "cores/RetroEngine/streams/RetroEngineStreamSwFramebuffer.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CRetroEngine::CRetroEngine(CRetroEngineGuiBridge& guiBridge, const std::string& savestatePath)
  : m_guiBridge(guiBridge),
    m_savestatePath(savestatePath),
    m_streamManager(std::make_unique<CRetroEngineStreamManager>()),
    m_renderer(std::make_unique<CRetroEngineRenderer>(m_guiBridge, *m_streamManager))
{
}

CRetroEngine::~CRetroEngine() = default;

void CRetroEngine::Initialize()
{
  // Initialize rendering
  m_renderer->Initialize();
}

void CRetroEngine::Deinitialize()
{
  // Deinitialize stream
  if (m_stream)
    m_streamManager->CloseStream(std::move(m_stream));

  // Deinitialize rendering
  m_renderer->Deinitialize();
}
