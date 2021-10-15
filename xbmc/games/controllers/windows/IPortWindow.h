/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "input/InputTypes.h"
#include "input/joysticks/JoystickTypes.h"

#include <string>
#include <vector>

class CEvent;

/*!
 * \brief Controller port configuration window
 *
 * The configuration window presents a list of ports and their attached
 * controllers.
 *
 * The label2 of each port is the currently-connected controller. The user
 * selects from all controllers that the port accepts (as given by topology.xml).
 *
 * The controller topology is stored as a generic tree, but here we apply
 * game logic to simplify controller selection.
 */

namespace KODI
{
namespace GAME
{
/*!
 * \brief A list populated by controller ports
 */
class IPortList
{
public:
  virtual ~IPortList() = default;

  /*!
   * \brief Initialize the resource
   *
   * \return true if the resource is initialized and can be used
   *         false if the resource failed to initialize and must not be used
   */
  virtual bool Initialize() = 0;

  /*!
   * \brief Deinitialize the resource
   */
  virtual void Deinitialize() = 0;

  /*!
   * \brief Refresh the contents of the list
   *
   * \return True if the list was changed, false otherwise
   */
  virtual void Refresh() = 0;

  /*!
   * \brief The specified port has been focused
   *
   * \param buttonIndex The index of the port's button
   */
  virtual void OnFocus(unsigned int buttonIndex) = 0;

  /*!
   * \brief The specified port has been selected
   *
   * \param buttonIndex The index of the port's button
   */
  virtual void OnSelect(unsigned int buttonIndex) = 0;

  /*!
   * \brief Reset the focused port
   */
  virtual void ResetPorts() = 0;
};
} // namespace GAME
} // namespace KODI
