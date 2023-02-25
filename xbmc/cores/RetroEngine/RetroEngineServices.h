/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace PERIPHERALS
{
class CPeripherals;
}

namespace KODI
{
namespace GAME
{
class CGameServices;
}

namespace RETRO_ENGINE
{
class CRetroEngineGuiBridge;
class CRetroEngineGuiManager;
class CRetroEngineInputManager;

class CRetroEngineServices
{
public:
  CRetroEngineServices(PERIPHERALS::CPeripherals& peripheralManager);
  ~CRetroEngineServices();

  void Initialize(GAME::CGameServices& gameServices);
  void Deinitialize();

  // RetroEngine subsystems
  CRetroEngineGuiBridge& GuiBridge(const std::string& savestatePath);
  CRetroEngineInputManager& InputManager() { return *m_inputManager; }

private:
  // Subsystems
  std::unique_ptr<CRetroEngineGuiManager> m_guiManager;
  std::unique_ptr<CRetroEngineInputManager> m_inputManager;
};

} // namespace RETRO_ENGINE
} // namespace KODI
