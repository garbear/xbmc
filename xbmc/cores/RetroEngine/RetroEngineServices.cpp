/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroEngine/RetroEngineServices.h"

#include "ServiceBroker.h"
#include "application/AppParams.h"
#include "cores/RetroEngine/guibridge/RetroEngineGuiBridge.h"
#include "cores/RetroEngine/guibridge/RetroEngineGuiManager.h"
#include "cores/RetroEngine/input/RetroEngineInputManager.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CRetroEngineServices::CRetroEngineServices(PERIPHERALS::CPeripherals& peripheralManager)
  : m_guiManager(std::make_unique<CRetroEngineGuiManager>()),
    m_inputManager(std::make_unique<CRetroEngineInputManager>(peripheralManager))
{
}

CRetroEngineServices::~CRetroEngineServices() = default;

void CRetroEngineServices::Initialize(GAME::CGameServices& gameServices)
{
  CLog::Log(LOGDEBUG, "RETROENGINE: Initializing services");

  m_inputManager->Initialize(gameServices);
}

void CRetroEngineServices::Deinitialize()
{
  CLog::Log(LOGDEBUG, "RETROENGINE: Deinitializing services");

  m_inputManager->Deinitialize();
}

CRetroEngineGuiBridge& CRetroEngineServices::GuiBridge(const std::string& savestatePath)
{
  return m_guiManager->GetGuiBridge(savestatePath);
}
