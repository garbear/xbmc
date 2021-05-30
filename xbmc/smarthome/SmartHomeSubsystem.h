/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/streams/SmartHomeStreams.h" // TODO: Move to cpp

#include <memory>

namespace KODI
{
namespace SMART_HOME
{
class CSmartHomeServices;
class CSmartHomeStreams;

struct SmartHomeSubsystems
{
  std::unique_ptr<CSmartHomeStreams> Streams;
};

/*!
 * \brief Base class for game client subsystems
 */
class CSmartHomeSubsystem
{
public:
  /*!
   * \brief Create a struct with the allocated subsystems
   *
   * \param gameClient The owner of the subsystems
   *
   * \return A fully-allocated SmartHomeSubsystems struct
   */
  static SmartHomeSubsystems CreateSubsystems(CSmartHomeServices& smartHome);

  /*!
   * \brief Deallocate subsystems
   *
   * \param subsystems The subsystems created by CreateSubsystems()
   */
  static void DestroySubsystems(SmartHomeSubsystems& subsystems);
};

} // namespace SMART_HOME
} // namespace KODI
