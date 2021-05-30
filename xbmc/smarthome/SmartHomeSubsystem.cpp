/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeSubsystem.h"

#include "smarthome/SmartHomeServices.h"

using namespace KODI;
using namespace SMART_HOME;

SmartHomeSubsystems CSmartHomeSubsystem::CreateSubsystems(CSmartHomeServices& smartHome)
{
  SmartHomeSubsystems subsystems = {};

  subsystems.Streams.reset(new CSmartHomeStreams(smartHome));

  return subsystems;
}

void CSmartHomeSubsystem::DestroySubsystems(SmartHomeSubsystems& subsystems)
{
  subsystems.Streams.reset();
}
