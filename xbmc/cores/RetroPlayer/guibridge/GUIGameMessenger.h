/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CGUIComponent;

namespace KODI
{
namespace RETRO
{
class CRPProcessInfo;

/*!
 * \brief Class to send messages to the GUI, if a GUI is present
 */
class CGUIGameMessenger
{
public:
  CGUIGameMessenger(CRPProcessInfo& processInfo);

  /*!
   * \brief Refresh GUI elements being displayed for the given savestate
   *
   * \param savestatePath The savestate to refresh
   */
  void RefreshSavestates(const std::string& savestatePath);

private:
  CGUIComponent* const m_guiComponent;
};
} // namespace RETRO
} // namespace KODI
