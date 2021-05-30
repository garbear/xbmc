/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CAppParamParser;

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

namespace SMART_HOME
{
class CRos2;
class CSmartHomeGuiBridge;
class CSmartHomeGuiManager;
class CSmartHomeInputManager;

class CSmartHomeServices
{
public:
  CSmartHomeServices(const CAppParamParser& params, PERIPHERALS::CPeripherals& peripheralManager);
  ~CSmartHomeServices();

  void Initialize(GAME::CGameServices& gameServices);
  void Deinitialize();

  // Smart home subsystems
  CRos2& Ros2() { return *m_ros2; }
  CSmartHomeGuiBridge& GuiBridge(const std::string& pubSubTopic);

  //! @todo Remove GUI dependency
  void FrameMove();

private:
  // Subsystems
  std::unique_ptr<CSmartHomeGuiManager> m_guiManager;
  std::unique_ptr<CSmartHomeInputManager> m_inputManager;
  std::unique_ptr<CRos2> m_ros2;
};

} // namespace SMART_HOME
} // namespace KODI
