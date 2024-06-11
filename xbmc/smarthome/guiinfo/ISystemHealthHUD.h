/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

  /*!
   * \brief Returns true if the system has been used recently, false otherwise
   *
   * \param systemName The name of the system
   *
   * \return true if the system has been used recently, false otherwise
   */
  virtual bool IsActive(const std::string& systemName) const = 0;

  /*!
   * \brief Get the temperature of the system, in degrees Celsius
   *
   * \param systemName The name of the system
   *
   * \return The temperature of the system, in degrees Celsius
   */
  virtual float TemperatureDegC(const std::string& systemName) const = 0;
};
} // namespace SMART_HOME
} // namespace KODI
