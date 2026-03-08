/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameClientDiscModel, AddDiscAndMarkRemovedKeepsSize)
{
  // Verify removing a disc marks a tombstone without shrinking the model
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));

  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_EQ(model.Size(), 2U);
  EXPECT_TRUE(model.IsRemovedSlotByIndex(0));
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
}

TEST(TestGameClientDiscModel, RemovedSlotClearsPathAndLabel)
{
  // Verify removed slots no longer expose path/label information
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/game/disc1.iso", "Disc One"));
  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_EQ(model.GetPathByIndex(0), "");
  EXPECT_EQ(model.GetLabelByIndex(0), "");
}

TEST(TestGameClientDiscModel, LookupIgnoresRemovedSlots)
{
  // Verify removed slots are ignored by path/basename lookup
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/game/disc1.iso"));
  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_FALSE(model.GetDiscIndexByPath("/roms/game/disc1.iso").has_value());
  EXPECT_FALSE(model.GetDiscIndexByBasename("disc1.iso").has_value());
}

TEST(TestGameClientDiscModel, MultipleRemovedSlotsCanCoexist)
{
  // Verify multiple tombstones can coexist and preserve index slots
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));

  ASSERT_TRUE(model.MarkRemovedByIndex(0));
  ASSERT_TRUE(model.MarkRemovedByIndex(2));

  EXPECT_EQ(model.Size(), 3U);
  EXPECT_TRUE(model.IsRemovedSlotByIndex(0));
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
  EXPECT_TRUE(model.IsRemovedSlotByIndex(2));
}

TEST(TestGameClientDiscModel, SelectionReplacementSkipsRemovedSlots)
{
  // Verify selected replacement skips removed tombstones
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));

  ASSERT_TRUE(model.SetSelectedDiscByIndex(1));
  ASSERT_TRUE(model.MarkRemovedByIndex(1));

  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_FALSE(model.GetSelectedDiscIndex().has_value());
}

TEST(TestGameClientDiscModel, RemovingOnlySelectedDiscClearsSelection)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.SetSelectedDiscByIndex(0));

  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_FALSE(model.GetSelectedDiscIndex().has_value());
}

TEST(TestGameClientDiscModel, MixedSlotModelBehavior)
{
  // Verify mixed Disc/RemovedSlot behavior for labels and paths
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddRemovedSlot());
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd", "Disc 3"));
  ASSERT_TRUE(model.MarkRemovedByIndex(2));

  EXPECT_EQ(model.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(model.GetLabelByIndex(0), "disc1.chd");
  EXPECT_EQ(model.GetPathByIndex(1), "");
  EXPECT_EQ(model.GetLabelByIndex(1), "");
  EXPECT_EQ(model.GetPathByIndex(2), "");
  EXPECT_EQ(model.GetLabelByIndex(2), "");
}

TEST(TestGameClientDiscModel, RemovedSlotsAreNotSelectable)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddRemovedSlot());

  EXPECT_TRUE(model.IsSelectableSlotByIndex(0));
  EXPECT_FALSE(model.IsSelectableSlotByIndex(1));
}

TEST(TestGameClientDiscModel, MergePreservesRemovedTombstoneWhenCoreReportsRemoved)
{
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previousDiscs, coreDiscs);

  ASSERT_EQ(merged.size(), 2U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
}

TEST(TestGameClientDiscModel, MergeDoesNotDuplicateRemovedTombstone)
{
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previousDiscs, coreDiscs);

  size_t removedCount = 0;
  size_t discCount = 0;
  for (const auto& disc : merged)
  {
    if (disc.slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
      ++removedCount;
    else if (disc.slotType == GameClientDiscEntry::DiscSlotType::Disc)
      ++discCount;
  }

  EXPECT_EQ(removedCount, 1U);
  EXPECT_EQ(discCount, 1U);
}

TEST(TestGameClientDiscModel, MergePreservesTrailingRemovedSlotsWhenCoreShrinks)
{
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc1.chd", "disc1.chd", ""},
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""}};

  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc1.chd", "disc1.chd", ""}};

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previousDiscs, coreDiscs);

  ASSERT_EQ(merged.size(), 3U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged[2].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
}

TEST(TestGameClientDiscModel, MergeDropsTrailingNonRemovedEntriesWhenCoreShrinks)
{
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc1.chd", "disc1.chd", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc3.chd", "disc3.chd", ""},
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""}};

  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc1.chd", "disc1.chd", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previousDiscs, coreDiscs);

  ASSERT_EQ(merged.size(), 3U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[0].path, "/roms/disc1.chd");
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[1].path, "/roms/disc2.chd");
  EXPECT_EQ(merged[2].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
}

TEST(TestGameClientDiscModel, ClearResetsEjectedState)
{
  CGameClientDiscModel model;

  model.SetEjected(true);
  ASSERT_TRUE(model.IsEjected());

  model.Clear();

  EXPECT_FALSE(model.IsEjected());
}
