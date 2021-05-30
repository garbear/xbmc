/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeServices.h"

#include "AppParamParser.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/streams/RPStreamManager.h"
#include "smarthome/Ros2.h"
#include "smarthome/SmartHomeSubsystem.h"
#include "utils/log.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeServices::CSmartHomeServices(const CAppParamParser& params)
  : m_subsystems(CSmartHomeSubsystem::CreateSubsystems(*this)),
    m_ros2(std::make_unique<CRos2>(params.GetArgs(), Streams())),
    m_gameRenderManager(std::make_unique<RETRO::CGUIGameRenderManager>())
{
}

CSmartHomeServices::~CSmartHomeServices() = default;

void CSmartHomeServices::Initialize()
{
  m_processInfo.reset(RETRO::CRPProcessInfo::CreateInstance());
  if (!m_processInfo)
  {
    CLog::Log(LOGERROR, "SMARTHOME: Failed to create - no process info registered");
  }
  else
  {
    m_renderManager = std::make_unique<RETRO::CRPRenderManager>(*m_processInfo);
    m_streamManager = std::make_unique<RETRO::CRPStreamManager>(*m_renderManager, *m_processInfo);

    Streams().Initialize(*m_streamManager);

    m_gameRenderManager->RegisterPlayer(m_renderManager->GetGUIRenderTargetFactory(),
                                        m_renderManager.get(), nullptr);

    CLog::Log(LOGDEBUG, "SMARTHOME: Created renderer");
  }

  m_ros2->Initialize();
}

void CSmartHomeServices::Deinitialize()
{
  m_ros2->Deinitialize();

  m_gameRenderManager->UnregisterPlayer();

  Streams().Deinitialize();

  m_streamManager.reset();
  m_renderManager.reset();
  m_processInfo.reset();
}

void CSmartHomeServices::FrameMove()
{
  if (m_renderManager)
    m_renderManager->FrameMove();
}

std::shared_ptr<RETRO::CGUIRenderHandle> CSmartHomeServices::RegisterControl(
    RETRO::CGUIGameControl& control)
{
  if (m_gameRenderManager)
    return m_gameRenderManager->RegisterControl(control);

  return std::shared_ptr<RETRO::CGUIRenderHandle>();
}
