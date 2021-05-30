/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeServices.h"

#include "AppParamParser.h"
#include "smarthome/guibridge/SmartHomeGuiBridge.h"
#include "smarthome/ros2/Ros2.h"
#include "utils/log.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeServices::CSmartHomeServices(const CAppParamParser& params)
  : m_guiBridge(std::make_unique<CSmartHomeGuiBridge>()),
    m_ros2(std::make_unique<CRos2>(*m_guiBridge, params.GetArgs()))
{
}

CSmartHomeServices::~CSmartHomeServices() = default;

void CSmartHomeServices::Initialize()
{
  CLog::Log(LOGDEBUG, "SMARTHOME: Initializing services");

  m_ros2->Initialize();
}

void CSmartHomeServices::Deinitialize()
{
  CLog::Log(LOGDEBUG, "SMARTHOME: Deinitializing services");

  m_ros2->Deinitialize();
}

void CSmartHomeServices::FrameMove()
{
  //! @todo Remove GUI dependency
  m_ros2->FrameMove();
}
