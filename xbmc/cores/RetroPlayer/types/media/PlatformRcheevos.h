/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Platform.h"

namespace KODI
{
namespace RETRO
{
  class PlatformRcheevos
  {
  public:
    static Platform TranslateGameConsole(unsigned int gameConsole);
    static unsigned int TranslatePlatform(Platform platform);
  };
}
}
