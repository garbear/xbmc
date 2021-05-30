/*
 *  Copyright (C) 2024-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/SmartHomeProperty.h"
#include "utils/Temperature.h"

#include <chrono>
#include <optional>
#include <string>

namespace KODI
{
namespace SMART_HOME
{
/*!
 * \brief Interface exposing information to be used in a HUD (heads up display)
 * for system health
 */
class ISystemHealthHUD
{
public:
  virtual ~ISystemHealthHUD() = default;

  static constexpr std::chrono::milliseconds DefaultActiveTimeout()
  {
    return std::chrono::milliseconds{10000};
  }

  /*!
   * \brief Returns true if the system has been used recently, false otherwise
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   * \param timeout The telemetry timeout window
   *
   * \return true if the system has been used recently, false otherwise
   */
  virtual bool IsActive(const std::string& systemName, std::chrono::milliseconds timeout) = 0;

  /*!
   * \brief Returns true if the system has been used recently using the
   * default timeout window
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   *
   * \return true if the system has been used recently, false otherwise
   */
  virtual bool IsActive(const std::string& systemName)
  {
    return IsActive(systemName, DefaultActiveTimeout());
  }

  /*!
   * \brief Get the temperature of the system's CPU, in degrees Celsius
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   *
   * \return The temperature of the system, in degrees Celsius
   */
  virtual CTemperature CPUTemperature(const std::string& systemName) = 0;

  /*!
   * \brief Get the CPU utilization, as a percent
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   *
   * \return The CPU utilization, as a percent, or std::nullopt if unavailable
   */
  virtual std::optional<float> CPUUtilization(const std::string& systemName) = 0;

  /*!
   * \brief Get the CPU frequency, in Hertz
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   *
   * \return The CPU frequency, in Hertz, or std::nullopt if unavailable
   */
  virtual std::optional<double> CPUFrequencyHz(const std::string& systemName) = 0;

  /*!
   * \brief Get the RAM utilization, as a percent
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   *
   * \return The RAM utilization, as a percent, or std::nullopt if unavailable
   */
  virtual std::optional<float> MemoryUtilization(const std::string& systemName) = 0;

  /*!
   * \brief Get the system power, in Watts
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   *
   * \return Platform power in watts, or std::nullopt if unavailable
   */
  virtual std::optional<float> SystemPower(const std::string& systemName) = 0;

  /*!
   * \brief Get the input voltage of the system, in Volts
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   *
   * \return Input voltage in volts, or std::nullopt if unavailable
   */
  virtual std::optional<float> InputVoltage(const std::string& systemName) = 0;

  /*!
   * \brief Get the battery's current charge, as a percent
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   *
   * \return The battery charge, as a percent
   */
  virtual std::optional<float> BatteryCharge(const std::string& systemName) = 0;

  /*!
   * \brief Get the current load provided by the battery, in Watts
   *
   * The system will be subscribed to if not already subscribed.
   *
   * \param systemName The name of the system
   *
   * \return The battery load in Watts, or std::nullopt if unavailable
   *
   */
  virtual std::optional<float> BatteryLoad(const std::string& systemName) = 0;

  virtual std::optional<SmartHomePropertyValue> Property(const std::string& systemName,
                                                         const std::string& propertyName,
                                                         SmartHomePropertyType type) = 0;
};
} // namespace SMART_HOME
} // namespace KODI
