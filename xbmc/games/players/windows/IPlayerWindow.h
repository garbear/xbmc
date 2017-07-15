/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

/*!
 * \brief Player window
 *
 * The player window allows the player to configure the virtual controller
 * ports and assign players to the virtual ports.
 *
 * The centerpiece of a good player window is the grid of virtual ports. This
 * presents all of the hardware connected to the virtual console. Each port is
 * assigned a column for up to three input devices.
 *
 * Parallel to the virtual port grid is the player grid. This grid contains a
 * column for each virtual port. The window has two modes: In "exclusive ports"
 * mode, there is one row and players are excluded from occupied ports. When
 * this mode is disabled, a second row is shown to indicate that multiple
 * players can be used to control a single port, required by some handhelds and
 * turn-based multiplayer games.
 *
 * On the side is a column of spectators where player icons are placed when they
 * aren't connected to any virtual port.
 *
 * Navigation between grids is controlled to best follow the direction of motion.
 */

#include "games/controllers/ControllerTypes.h"
#include "games/GameTypes.h"

namespace KODI
{
  namespace GAME
  {
    class CControllerGrid;

    /*!
     * \brief Control for holding player avatars
     */
    class IPlayerPanel
    {
    public:
      virtual ~IPlayerPanel() = default;

      /*!
       * \brief A player (or empty slot) has been selected
       *
       * \param index The index of the player or empty slot
       */
      virtual void OnSelect(unsigned int index) = 0;

      /*!
       * \brief Set the maximum number of players in this panel
       */
      virtual void SetPlayerCount(unsigned int playerCount) = 0;

      /*!
       * \brief Set the size of avatars
       *
       * This is provided so that avatar sizes can match controller sizes after
       * resizing the controller topology to fit the control.
       */
      virtual void SetAvatarSize(unsigned int sizePx) = 0;
    };

    /*!
     * \brief Control for holding controller topology
     */
    class IControllerPanel
    {
    public:
      virtual ~IControllerPanel() = default;

      /*!
       * \brief Set controller grid
       */
      virtual void SetGrid(const CControllerGrid &grid) = 0;

      /*!
       * \brief Get the size of a controller icon
       */
      virtual unsigned int GetItemSize() const = 0;

      /*!
       * \brief A device has been selected
       *
       * \param column The column index of the device
       * \param row    The row index of the device
       */
      virtual void OnSelect(unsigned int column, unsigned int row) = 0;

      //virtual void FocusFromPlayerGrid(unsigned int col) = 0;

      /*!
       * \brief Get controller profiles for the specified location
       */
      virtual ControllerVector GetControllers(unsigned int column, unsigned int row) = 0;
    };

    /*!
     * \brief Player window
     */
    class IPlayerWindow
    {
    public:
      virtual ~IPlayerWindow() = default;
    };
  }
}
