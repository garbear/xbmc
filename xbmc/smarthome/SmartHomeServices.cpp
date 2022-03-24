/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeServices.h"

#include "AppParamParser.h"
#include "smarthome/guibridge/SmartHomeGuiBridge.h"
#include "smarthome/guibridge/SmartHomeGuiManager.h"
#include "smarthome/input/SmartHomeInputManager.h"
#include "smarthome/ros2/Ros2.h"
#include "utils/log.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeServices::CSmartHomeServices(const CAppParamParser& params,
                                       PERIPHERALS::CPeripherals& peripheralManager)
  : m_guiManager(std::make_unique<CSmartHomeGuiManager>()),
    m_inputManager(std::make_unique<CSmartHomeInputManager>(peripheralManager)),
    m_ros2(std::make_unique<CRos2>(*m_guiManager, *m_inputManager, params.GetArgs()))
{
}

CSmartHomeServices::~CSmartHomeServices() = default;

void CSmartHomeServices::Initialize(GAME::CGameServices& gameServices)
{
  CLog::Log(LOGDEBUG, "SMARTHOME: Initializing services");

  m_inputManager->Initialize(gameServices);
  m_ros2->Initialize();
}

void CSmartHomeServices::Deinitialize()
{
  CLog::Log(LOGDEBUG, "SMARTHOME: Deinitializing services");

  m_ros2->Deinitialize();
  m_inputManager->Deinitialize();
}

CSmartHomeGuiBridge& CSmartHomeServices::GuiBridge(const std::string& pubSubTopic)
{
  return m_guiManager->GetGuiBridge(pubSubTopic);
}

void CSmartHomeServices::FrameMove()
{
  //! @todo Remove GUI dependency
  m_ros2->FrameMove();
}
