/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameEngineConfig.h"

#include "cores/RetroEngine/guicontrols/GUIGameEngineControl.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CGUIGameEngineConfig::CGUIGameEngineConfig() = default;

CGUIGameEngineConfig::CGUIGameEngineConfig(const CGUIGameEngineConfig& other)
  : m_savestatePath(other.m_savestatePath)
{
}

CGUIGameEngineConfig::~CGUIGameEngineConfig() = default;

void CGUIGameEngineConfig::Reset()
{
  m_savestatePath.clear();
}
