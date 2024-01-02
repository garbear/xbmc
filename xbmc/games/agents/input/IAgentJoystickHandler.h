/*
 *  Copyright (C) 2022-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace GAME
{
class IAgentJoystickHandler;
/*!
 * \ingroup games
 *
 * \brief Handles game controller events
 */
class IAgentJoystickHandler
{
public:
  virtual ~IAgentJoystickHandler() = default;

  /*!
   * \brief
   */
  virtual void OnPortIncrease() = 0;

  /*!
   * \brief
   */
  virtual void OnPortDecrease() = 0;
};
} // namespace GAME
} // namespace KODI
