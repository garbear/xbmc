/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/ImageSubscriptionKey.h"
#include "threads/IRunnable.h"

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

class CThread;

namespace PERIPHERALS
{
class CPeripheral;
}

namespace rclcpp
{
class Node;
class SubscriptionBase;
class TimerBase;
namespace executors
{
class MultiThreadedExecutor;
} // namespace executors
} // namespace rclcpp

namespace KODI
{
namespace SMART_HOME
{
class CRos2InputPublisher;
class CRos2NowPlayingPublisher;
class CRos2PeripheralSubscriber;
class CRos2PowerMeterManager;
class CRos2SystemHealthManager;
class CRos2VehicleManager;
class CRos2VideoSubscription;
class CSmartHomeGuiBridge;
class CSmartHomeInputManager;
class IPowerMeterHUD;
class ISystemHealthHUD;
class IVehicleHUD;

class CRos2Node : public IRunnable
{
public:
  CRos2Node(CSmartHomeInputManager& inputManager);
  virtual ~CRos2Node();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  std::shared_ptr<const PERIPHERALS::CPeripheral> GetRemotePeripheral(
      const std::string& hostname, const std::string& address) const;

  std::optional<int> GetControllerBattery(const std::string& hostname) const;

  // GUI interface
  void RegisterImageTopic(CSmartHomeGuiBridge& guiBridge, const ImageSubscriptionKey& subscription);
  void UnregisterImageTopic(const ImageSubscriptionKey& subscription);
  ISystemHealthHUD* GetSystemHealthHUD() const;
  IPowerMeterHUD* GetPowerMeterHUD() const;
  IVehicleHUD* GetVehicleHUD() const;

  //! @todo Remove GUI dependency
  void FrameMove();

private:
  void DiscoverPeripheralPublishers();

  // Implementation of IRunnable
  void Run() override;

  // Construction parameters
  CSmartHomeInputManager& m_inputManager;
  const std::string m_rosNamespace;

  // ROS parameters
  std::shared_ptr<rclcpp::executors::MultiThreadedExecutor> m_executor;
  std::shared_ptr<rclcpp::Node> m_node;

  struct PeripheralSubscription
  {
    std::shared_ptr<CRos2PeripheralSubscriber> devices;
    std::shared_ptr<rclcpp::SubscriptionBase> subscription;
  };

  mutable std::mutex m_peripheralMutex;
  std::map<std::string, PeripheralSubscription> m_peripheralSubs;
  std::string m_localHostname;
  std::string m_localPeripheralTopic;
  std::shared_ptr<rclcpp::TimerBase> m_discoveryTimer;

  struct VideoSubscription
  {
    std::unique_ptr<CRos2VideoSubscription> subscription;
    unsigned int referenceCount{1};
  };

  // Subscription identity -> owned subscriber and number of camera controls
  std::map<ImageSubscriptionKey, VideoSubscription> m_videoSubs;
  std::unique_ptr<CRos2InputPublisher> m_peripheralPublisher;
  std::unique_ptr<CRos2SystemHealthManager> m_systemHealthManager;
  std::unique_ptr<CRos2PowerMeterManager> m_powerMeterManager;
  std::unique_ptr<CRos2VehicleManager> m_vehicleManager;
  std::unique_ptr<CRos2NowPlayingPublisher> m_nowPlayingPublisher;

  // Threading parameters
  std::unique_ptr<CThread> m_thread;
};
} // namespace SMART_HOME
} // namespace KODI
