/*
 *  Copyright (C) 2017-2023 Team Kodi
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
/*!
 * \ingroup games
 *
 * \brief Class to represent a game player (a.k.a. agent)
 *
 * The term "agent" is used to distinguish game players from the myriad of other
 * uses of the term "player" in Kodi, such as media players and player cores.
 */
class CGameAgent
{
public:
  CGameAgent();
  ~CGameAgent();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();
};

} // namespace GAME
} // namespace KODI
