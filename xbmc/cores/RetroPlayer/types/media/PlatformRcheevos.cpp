/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Platform.h"

//#include <rcheevos.h>

using namespace KODI;
using namespace RETRO;

Platform TranslateGameConsole(unsigned int gameConsole)
{
  /*! TODO
  switch (gameConsole)
  {
  case RC_CONSOLE_MEGA_DRIVE: return Platform::MEGA_DRIVE;
  case RC_CONSOLE_NINTENDO_64: return Platform::NINTENDO_64;
  case RC_CONSOLE_SUPER_NINTENDO: return Platform::SUPER_NINTENDO;
  case RC_CONSOLE_GAMEBOY: return Platform::GAMEBOY;
  case RC_CONSOLE_GAMEBOY_ADVANCE: return Platform::GAMEBOY_ADVANCE;
  case RC_CONSOLE_GAMEBOY_COLOR: return Platform::GAMEBOY_COLOR;
  case RC_CONSOLE_NINTENDO: return Platform::NINTENDO;
  case RC_CONSOLE_PC_ENGINE: return Platform::PC_ENGINE;
  case RC_CONSOLE_SEGA_CD: return Platform::SEGA_CD;
  case RC_CONSOLE_SEGA_32X: return Platform::SEGA_32X;
  case RC_CONSOLE_MASTER_SYSTEM: return Platform::MASTER_SYSTEM;
  case RC_CONSOLE_PLAYSTATION: return Platform::PLAYSTATION;
  case RC_CONSOLE_ATARI_LYNX: return Platform::ATARI_LYNX;
  case RC_CONSOLE_NEOGEO_POCKET: return Platform::NEOGEO_POCKET;
  case RC_CONSOLE_GAME_GEAR: return Platform::GAME_GEAR;
  case RC_CONSOLE_GAMECUBE: return Platform::GAMECUBE;
  case RC_CONSOLE_ATARI_JAGUAR: return Platform::ATARI_JAGUAR;
  case RC_CONSOLE_NINTENDO_DS: return Platform::NINTENDO_DS;
  case RC_CONSOLE_WII: return Platform::WII;
  case RC_CONSOLE_WII_U: return Platform::WII_U;
  case RC_CONSOLE_PLAYSTATION_2: return Platform::PLAYSTATION_2;
  case RC_CONSOLE_XBOX: return Platform::XBOX;
  case RC_CONSOLE_SKYNET: return Platform::SKYNET;
  case RC_CONSOLE_XBOX_ONE: return Platform::XBOX_ONE;
  case RC_CONSOLE_ATARI_2600: return Platform::ATARI_2600;
  case RC_CONSOLE_MS_DOS: return Platform::MS_DOS;
  case RC_CONSOLE_ARCADE: return Platform::ARCADE;
  case RC_CONSOLE_VIRTUAL_BOY: return Platform::VIRTUAL_BOY;
  case RC_CONSOLE_MSX: return Platform::MSX;
  case RC_CONSOLE_COMMODORE_64: return Platform::COMMODORE_64;
  case RC_CONSOLE_ZX81: return Platform::ZX81;
  default:
    break;
  }
  */

  return Platform::UNKNOWN;
}
