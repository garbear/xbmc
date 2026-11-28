/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRos2.h"

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SMART_HOME
{

class CRos2Node;
class CSmartHomeGuiManager;
class CSmartHomeInputManager;

class CRos2 : public IRos2
{
public:
  CRos2(CSmartHomeGuiManager& guiManager,
        CSmartHomeInputManager& inputManager,
        std::vector<std::string> cmdLineArgs);
  ~CRos2() override;

  // Implementation of IRos2
  void Initialize() override;
  void Deinitialize() override;
  std::shared_ptr<const PERIPHERALS::CPeripheral> GetRemotePeripheral(
      const std::string& hostname, const std::string& address) const override;
  std::optional<int> GetControllerBattery(const std::string& hostname) const override;
  void RegisterImageTopic(const ImageSubscriptionKey& subscription) override;
  void UnregisterImageTopic(const ImageSubscriptionKey& subscription) override;
  ISystemHealthHUD* GetSystemHealthHUD() const override;
  IPowerMeterHUD* GetPowerMeterHUD() const override;
  virtual IVehicleHUD* GetVehicleHUD() const override;
  void FrameMove() override;

private:
  // Construction parameters
  CSmartHomeGuiManager& m_guiManager;
  CSmartHomeInputManager& m_inputManager;
  std::vector<std::string> m_cmdLineArgs;

  // ROS parameters
  std::unique_ptr<CRos2Node> m_node;
};
} // namespace SMART_HOME
} // namespace KODI
