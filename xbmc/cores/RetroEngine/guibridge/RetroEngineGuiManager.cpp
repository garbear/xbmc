/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroEngineGuiManager.h"

#include "RetroEngineGuiBridge.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CRetroEngineGuiManager::~CRetroEngineGuiManager() = default;

CRetroEngineGuiBridge& CRetroEngineGuiManager::GetGuiBridge(const std::string& savestatePath)
{
  if (m_guiBridges.find(savestatePath) == m_guiBridges.end())
    m_guiBridges[savestatePath] = std::make_unique<CRetroEngineGuiBridge>();

  return *m_guiBridges[savestatePath];
}
