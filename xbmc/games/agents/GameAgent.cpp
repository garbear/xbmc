/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameAgent.h"

#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace GAME;

CGameAgent::CGameAgent(PERIPHERALS::PeripheralPtr peripheral) : m_peripheral(std::move(peripheral))
{
}

CGameAgent::~CGameAgent() = default;

CDateTime CGameAgent::LastActive() const
{
  return m_peripheral->LastActive();
}
