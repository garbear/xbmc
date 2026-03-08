/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/disc/GameClientDiscModel.h"

#include <vector>

namespace KODI
{
namespace GAME
{

std::vector<GameClientDiscEntry> OverlayRemovedTombstonesByIndex(
    const std::vector<GameClientDiscEntry>& previousDiscs,
    const std::vector<GameClientDiscEntry>& coreDiscs);

} // namespace GAME
} // namespace KODI
