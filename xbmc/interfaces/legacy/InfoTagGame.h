/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "games/tags/GameInfoTag.h"

namespace XBMCAddon
{
namespace xbmc
{

///
/// \defgroup python_InfoTagGame InfoTagGame
/// \ingroup python_xbmc
/// @{
/// @brief **Kodi's game info tag class.**
///
/// \python_class{ InfoTagGame() }
///
/// To get game info tag data of currently played source.
///
/// @note Info tag load is only be possible from present player class.
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.py}
/// ...
/// tag = xbmc.Player().getGameInfoTag()
///
/// file  = tag.getFile()
/// ...
/// ~~~~~~~~~~~~~
///
class InfoTagGame : public AddonClass
{
private:
  KODI::GAME::CGameInfoTag* infoTag;

public:
#ifndef SWIG
  explicit InfoTagGame(const KODI::GAME::CGameInfoTag& tag);
#endif
  InfoTagGame();
  ~InfoTagGame() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getFile() }
  /// To get the game file name.
  ///
  /// @return [string] File name
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getFile();
#else
  String getFile();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getTitle() }
  /// To get the game title.
  ///
  /// @return [string] title
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getTitle();
#else
  String getTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getPlatform() }
  /// To get the platform of the game.
  ///
  /// @return [string] platform
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getPlatform();
#else
  String getPlatform();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getGenres() }
  /// To get the genres of the game.
  ///
  /// @return [string] genres
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getGenres();
#else
  String getGenres();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getDeveloper() }
  /// To get the developer of the game.
  ///
  /// @return [string] developer
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getDeveloper();
#else
  String getDeveloper();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getOverview() }
  /// To get the overview of the game.
  ///
  /// @return [string] overview
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getOverview();
#else
  String getOverview();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getYear() }
  /// To get the release year of the game.
  ///
  /// @return [int] year
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getYear();
#else
  int getYear();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getID() }
  /// To get the ID of the game.
  ///
  /// @return [string] ID
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getID();
#else
  String getID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getRegion() }
  /// To get the region of the game.
  ///
  /// @return [string] region
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getRegion();
#else
  String getRegion();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getPublisher() }
  /// To get the publisher of the game.
  ///
  /// @return [string] publisher
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getPublisher();
#else
  String getPublisher();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getFormat() }
  /// To get the format of the game (PAL/NTSC).
  ///
  /// @return [string] format
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getFormat();
#else
  String getFormat();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getCartridgeType() }
  /// To get the cartige type of the game, e.g. "ROM+MBC5+RAM+BATT" or "CD".
  ///
  /// @return [string] cartige type
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getCartridgeType();
#else
  String getCartridgeType();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getGameClient() }
  /// To get the id of game client used to play the game.
  ///
  /// @return [string] game client ID
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getGameClient();
#else
  String getGameClient();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getCaption() }
  /// To get a rich presence evaluation of the game's state.
  ///
  /// @return [string] publisher
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  ///
  getCaption();
#else
  String getCaption();
#endif
};
} // namespace xbmc
} // namespace XBMCAddon
