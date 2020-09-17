/*
 *  Copyright (C) 2012-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RConsoleIDs.h"

#include <map>
#include <string>

namespace KODI
{
namespace GAME
{
class CGameClient;
}

namespace RETRO
{
class CCheevos
{
public:
  CCheevos(GAME::CGameClient* gameClient, const std::string userName, const std::string loginToken);
  void ResetRuntime();
  void EnableRichPresence();
  bool GetRichPresenceEvaluation(char* evaluation, size_t size);

  std::string GetTitle() { return m_title; }
  std::string GetPublisher() { return m_publisher; }
  std::string GetDeveloper() { return m_developer; }
  std::string GetGenre() { return m_genre; }
  std::string GetConsoleName() { return m_consoleName; }
  std::string GetReleased() { return m_released; }

private:
  bool LoadData();

  GAME::CGameClient* const m_gameClient;
  std::string m_userName;
  std::string m_loginToken;
  std::string m_romHash{};
  std::string m_richPresenceScript{};
  unsigned m_gameID{};
  bool m_richPresenceLoaded{};

  // Game metadata
  std::string m_title;
  std::string m_publisher;
  std::string m_developer;
  std::string m_genre;
  std::string m_consoleName;
  std::string m_released;

  const std::map<std::string, int> m_extensionToConsole = {{".a26", RC_CONSOLE_ATARI_2600},
                                                           {".a78", RC_CONSOLE_ATARI_7800},
                                                           {".agb", RC_CONSOLE_GAMEBOY_ADVANCE},
                                                           {".cdi", RC_CONSOLE_DREAMCAST},
                                                           {".cdt", RC_CONSOLE_AMSTRAD_PC},
                                                           {".cgb", RC_CONSOLE_GAMEBOY_COLOR},
                                                           {".chd", RC_CONSOLE_DREAMCAST},
                                                           {".cpr", RC_CONSOLE_AMSTRAD_PC},
                                                           {".d64", RC_CONSOLE_COMMODORE_64},
                                                           {".gb", RC_CONSOLE_GAMEBOY},
                                                           {".gba", RC_CONSOLE_GAMEBOY_ADVANCE},
                                                           {".gbc", RC_CONSOLE_GAMEBOY_COLOR},
                                                           {".gdi", RC_CONSOLE_DREAMCAST},
                                                           {".j64", RC_CONSOLE_ATARI_JAGUAR},
                                                           {".jag", RC_CONSOLE_ATARI_JAGUAR},
                                                           {".lnx", RC_CONSOLE_ATARI_LYNX},
                                                           {".mds", RC_CONSOLE_SATURN},
                                                           {".min", RC_CONSOLE_POKEMON_MINI},
                                                           {".mx1", RC_CONSOLE_MSX},
                                                           {".mx2", RC_CONSOLE_MSX},
                                                           {".n64", RC_CONSOLE_NINTENDO_64},
                                                           {".ndd", RC_CONSOLE_NINTENDO_64},
                                                           {".nds", RC_CONSOLE_NINTENDO_DS},
                                                           {".nes", RC_CONSOLE_NINTENDO},
                                                           {".o", RC_CONSOLE_ATARI_LYNX},
                                                           {".pce", RC_CONSOLE_PC_ENGINE},
                                                           {".sfc", RC_CONSOLE_SUPER_NINTENDO},
                                                           {".sgx", RC_CONSOLE_PC_ENGINE},
                                                           {".smc", RC_CONSOLE_SUPER_NINTENDO},
                                                           {".sna", RC_CONSOLE_AMSTRAD_PC},
                                                           {".tap", RC_CONSOLE_AMSTRAD_PC},
                                                           {".u1", RC_CONSOLE_NINTENDO_64},
                                                           {".v64", RC_CONSOLE_NINTENDO_64},
                                                           {".vb", RC_CONSOLE_VIRTUAL_BOY},
                                                           {".vboy", RC_CONSOLE_VIRTUAL_BOY},
                                                           {".vec", RC_CONSOLE_VECTREX},
                                                           {".voc", RC_CONSOLE_AMSTRAD_PC},
                                                           {".z64", RC_CONSOLE_NINTENDO_64}};
};
} // namespace RETRO
} // namespace KODI
