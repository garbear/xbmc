/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscMergeUtils.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

std::vector<GameClientDiscEntry> KODI::GAME::OverlayRemovedTombstonesByIndex(
    const std::vector<GameClientDiscEntry>& previousDiscs,
    const std::vector<GameClientDiscEntry>& coreDiscs)
{
  std::vector<GameClientDiscEntry> discs;
  discs.reserve(coreDiscs.size() > previousDiscs.size() ? coreDiscs.size() : previousDiscs.size());

  const size_t overlapCount = std::min(previousDiscs.size(), coreDiscs.size());
  for (size_t i = 0; i < overlapCount; ++i)
  {
    const bool preserveRemoved =
        previousDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot &&
        coreDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot;

    if (preserveRemoved)
      discs.push_back(previousDiscs[i]);
    else
      discs.push_back(coreDiscs[i]);
  }

  for (size_t i = overlapCount; i < previousDiscs.size(); ++i)
  {
    if (previousDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
      discs.push_back(previousDiscs[i]);
  }

  for (size_t i = overlapCount; i < coreDiscs.size(); ++i)
    discs.push_back(coreDiscs[i]);

  return discs;
}
