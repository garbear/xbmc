/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief A list populated by agent avatars
 */
class IAgentAvatarList
{
public:
  virtual ~IAgentAvatarList() = default;

  /*!
   * \brief Initialize resources
   *
   * \param gameClient The game client providing the avatars
   *
   * \return True if the resource is initialized and can be used, false if the
   *         resource failed to initialize and must not be used
   */
  virtual bool Initialize(GameClientPtr gameClient) = 0;

  /*!
   * \brief Deinitialize resources
   */
  virtual void Deinitialize() = 0;

  /*!
   * \brief Refresh the contents of the list
   */
  virtual void Refresh() = 0;
};
} // namespace GAME
} // namespace KODI
