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
#include "smarthome/rendering/SmartHomeRendering.h"
#include "smarthome/ros2/Ros2.h"
#include "smarthome/streams/SmartHomeStreams.h"
#include "utils/log.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeServices::CSmartHomeServices(const CAppParamParser& params)
  : m_guiBridge(std::make_unique<CSmartHomeGuiBridge>()),
    m_streams(std::make_unique<CSmartHomeStreams>()),
    m_rendering(std::make_unique<CSmartHomeRendering>(*m_guiBridge, *m_streams)),
    m_ros2(std::make_unique<CRos2>(*m_streams, params.GetArgs()))
{
}

CSmartHomeServices::~CSmartHomeServices() = default;

void CSmartHomeServices::Initialize()
{
  CLog::Log(LOGDEBUG, "SMARTHOME: Initializing services");

  m_rendering->Initialize();

  m_ros2->Initialize();
}

void CSmartHomeServices::Deinitialize()
{
  CLog::Log(LOGDEBUG, "SMARTHOME: Deinitializing services");

  m_ros2->Deinitialize();

  m_rendering->Deinitialize();
}
