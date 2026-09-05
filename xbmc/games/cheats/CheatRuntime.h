/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CheatPack.h"

#include <mutex>
#include <string>

namespace KODI::GAME
{
class CGameClient;

/*!
 * \ingroup games
 *
 * \brief The cheats available to the game being played
 *
 * Held for the length of a game, so the OSD can say whether there is anything
 * to offer before the player asks.
 */
class CCheatRuntime
{
public:
  /*!
   * \brief Look for cheats for a game and hold what is found
   *
   * A game with no cheat file, or a cheats folder that has not been set, ends
   * up with nothing, which is how the OSD knows not to offer them.
   */
  void Load(const std::string& gamePath);

  //! \brief Forget the cheats and switch off any that were applied
  void Clear();

  //! \brief True while the game being played has cheats to offer
  bool HasCheats() const;

  //! \brief The cheats found for this game, and whether each is switched on
  std::vector<Cheat> GetCheats() const;

  /*!
   * \brief Switch one cheat on or off
   *
   * The whole set is re-sent to the client afterwards.
   */
  void SetEnabled(unsigned int index, bool enabled);

  //! \brief The client the cheats are applied to, for as long as it is playing
  void SetGameClient(CGameClient* gameClient);

private:
  //! \brief Send every cheat to the client, called under the lock
  void Apply();

  mutable std::mutex m_mutex;
  CCheatPack m_pack;
  std::vector<bool> m_enabled;
  CGameClient* m_gameClient{nullptr};

  //! Whether the client took the cheat calls. The operations are optional, and
  //! one that returns GAME_ERROR_NOT_IMPLEMENTED can never be cheated at.
  bool m_clientTakesCheats{false};
};
} // namespace KODI::GAME
