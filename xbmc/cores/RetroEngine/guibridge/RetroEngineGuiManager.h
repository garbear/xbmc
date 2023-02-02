/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <string>

namespace KODI
{
namespace RETRO_ENGINE
{
class CRetroEngineGuiBridge;

class CRetroEngineGuiManager
{
public:
  CRetroEngineGuiManager() = default;
  ~CRetroEngineGuiManager();

  // Get bridge between GUI and renderer
  CRetroEngineGuiBridge& GetGuiBridge(const std::string& savestatePath);

private:
  // RetroEngine parameters
  std::map<std::string, std::unique_ptr<CRetroEngineGuiBridge>> m_guiBridges; // Savestate -> bridge
};

} // namespace RETRO_ENGINE
} // namespace KODI
