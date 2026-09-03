/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "games/AchievementRuntime.h"
#include "games/dialogs/osd/LeaderboardUtils.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
LeaderboardEntry MakeEntry(unsigned int rank, const std::string& user, std::time_t submitted)
{
  LeaderboardEntry entry;
  entry.rank = rank;
  entry.username = user;
  entry.score = "0:24.13";
  entry.submitted = submitted;
  entry.isPlayer = (user == "chris");
  return entry;
}

std::string CacheFile()
{
  return CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem(
      "gameleaderboards.xml");
}
} // namespace

class TestLeaderboardUtils : public testing::Test
{
protected:
  void SetUp() override { XFILE::CFile::Delete(CacheFile()); }
  void TearDown() override { XFILE::CFile::Delete(CacheFile()); }
};

TEST_F(TestLeaderboardUtils, FormatsAScoreTheWayItsLeaderboardMeasures)
{
  // The same integer means different things on different leaderboards, which is
  // why the format travels with it
  EXPECT_EQ(FormatLeaderboardScore(9000, "SCORE"), "9000");
  EXPECT_EQ(FormatLeaderboardScore(9000, "TIME"), "2:30.00");
  EXPECT_EQ(FormatLeaderboardScore(9000, "FRAMES"), "2:30.00");
  EXPECT_EQ(FormatLeaderboardScore(90, "TIMESECS"), "1:30");
  EXPECT_EQ(FormatLeaderboardScore(9000, "FIXED1"), "900.0");
  EXPECT_EQ(FormatLeaderboardScore(9000, "FIXED2"), "90.00");
  EXPECT_EQ(FormatLeaderboardScore(9000, "FIXED3"), "9.000");

  // A unit we do not recognise is shown as a plain number: a wrong unit reads
  // worse than none
  EXPECT_EQ(FormatLeaderboardScore(9000, "SOMETHING_NEW"), "9000");
  EXPECT_EQ(FormatLeaderboardScore(9000, ""), "9000");
}

TEST_F(TestLeaderboardUtils, MedalsOnlyGoToTheTopThree)
{
  EXPECT_EQ(RankMedal(1), "gold");
  EXPECT_EQ(RankMedal(2), "silver");
  EXPECT_EQ(RankMedal(3), "bronze");
  EXPECT_TRUE(RankMedal(4).empty());

  // Rank zero means unranked, not first
  EXPECT_TRUE(RankMedal(0).empty());
}

TEST_F(TestLeaderboardUtils, ReadsBackWhatItKept)
{
  const std::time_t now = std::time(nullptr);

  std::vector<LeaderboardEntry> entries{MakeEntry(1, "Sonikku", now - 3600),
                                        MakeEntry(2, "TailsDoll", now - 90000),
                                        MakeEntry(3, "chris", now - 400000)};

  SaveLeaderboardEntries(7, entries);

  std::vector<LeaderboardEntry> back;
  ASSERT_TRUE(LoadLeaderboardEntries(7, back));
  ASSERT_EQ(back.size(), 3u);

  EXPECT_EQ(back[0].rank, 1u);
  EXPECT_EQ(back[0].username, "Sonikku");
  EXPECT_EQ(back[0].score, "0:24.13");
  EXPECT_EQ(back[0].submitted, now - 3600);
  EXPECT_FALSE(back[0].isPlayer);

  // The player's own row is what the header is composed from, so losing the
  // flag across a save would quietly break "your best"
  EXPECT_TRUE(back[2].isPlayer);
}

TEST_F(TestLeaderboardUtils, KnowsNothingAboutALeaderboardItNeverSaw)
{
  std::vector<LeaderboardEntry> back;
  EXPECT_FALSE(LoadLeaderboardEntries(999, back));
  EXPECT_TRUE(back.empty());
}

TEST_F(TestLeaderboardUtils, LookingTwiceLeavesOneRecordNotTwo)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, {MakeEntry(1, "Sonikku", now)});
  SaveLeaderboardEntries(7, {MakeEntry(1, "Knux", now), MakeEntry(2, "Sonikku", now)});

  std::vector<LeaderboardEntry> back;
  ASSERT_TRUE(LoadLeaderboardEntries(7, back));

  // Not four rows from two saves
  ASSERT_EQ(back.size(), 2u);
  EXPECT_EQ(back[0].username, "Knux");
}

TEST_F(TestLeaderboardUtils, KeepsLeaderboardsApart)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, {MakeEntry(1, "Sonikku", now)});
  SaveLeaderboardEntries(8, {MakeEntry(1, "Knux", now)});

  std::vector<LeaderboardEntry> seven;
  std::vector<LeaderboardEntry> eight;
  ASSERT_TRUE(LoadLeaderboardEntries(7, seven));
  ASSERT_TRUE(LoadLeaderboardEntries(8, eight));

  EXPECT_EQ(seven.front().username, "Sonikku");
  EXPECT_EQ(eight.front().username, "Knux");
}

TEST_F(TestLeaderboardUtils, ForgetsOneLeaderboardWithoutForgettingTheRest)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, {MakeEntry(1, "Sonikku", now)});
  SaveLeaderboardEntries(8, {MakeEntry(1, "Knux", now)});

  // What a submission does: the kept page for that one board is now wrong
  ForgetLeaderboardEntries(7);

  std::vector<LeaderboardEntry> gone;
  std::vector<LeaderboardEntry> kept;
  EXPECT_FALSE(LoadLeaderboardEntries(7, gone));
  EXPECT_TRUE(LoadLeaderboardEntries(8, kept));
}

TEST_F(TestLeaderboardUtils, ForgettingSomethingNeverKeptIsHarmless)
{
  const std::time_t now = std::time(nullptr);
  SaveLeaderboardEntries(7, {MakeEntry(1, "Sonikku", now)});

  ForgetLeaderboardEntries(999);

  std::vector<LeaderboardEntry> back;
  EXPECT_TRUE(LoadLeaderboardEntries(7, back));
}

TEST_F(TestLeaderboardUtils, ClearsEverythingWhenAsked)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, {MakeEntry(1, "Sonikku", now)});
  SaveLeaderboardEntries(8, {MakeEntry(1, "Knux", now)});

  ClearLeaderboardEntries();

  std::vector<LeaderboardEntry> back;
  EXPECT_FALSE(LoadLeaderboardEntries(7, back));
  EXPECT_FALSE(LoadLeaderboardEntries(8, back));

  // And clearing an empty cache is not an error
  ClearLeaderboardEntries();
}

TEST_F(TestLeaderboardUtils, RefusesStandingsThatHaveGoneStale)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, {MakeEntry(1, "Sonikku", now)});

  // Age the record past the freshness window by hand, since the test cannot
  // wait a week
  const std::string path = CacheFile();
  std::string xml;
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.Open(path));
    char buffer[8192] = {};
    const ssize_t read = file.Read(buffer, sizeof(buffer) - 1);
    ASSERT_GT(read, 0);
    xml.assign(buffer, static_cast<size_t>(read));
  }

  const std::string fetched = "fetched=\"" + std::to_string(now) + "\"";
  const std::string aged =
      "fetched=\"" + std::to_string(now - (8 * 24 * 60 * 60)) + "\""; // eight days
  ASSERT_NE(xml.find(fetched), std::string::npos);
  xml.replace(xml.find(fetched), fetched.size(), aged);

  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(path, true));
    file.Write(xml.c_str(), xml.size());
  }

  // A table a week old is asked for again rather than shown as if it were
  // current
  std::vector<LeaderboardEntry> back;
  EXPECT_FALSE(LoadLeaderboardEntries(7, back));
}

TEST_F(TestLeaderboardUtils, ScoreboardWritesTheStandingBackAndDropsTheStalePage)
{
  LeaderboardInfo leaderboard;
  leaderboard.id = 7;
  leaderboard.playerRank = 274;
  leaderboard.playerScore = "0:41.88";
  leaderboard.totalEntries = 1483;
  leaderboard.entries = {MakeEntry(1, "Sonikku", std::time(nullptr))};

  LeaderboardState state;
  state.leaderboards = {leaderboard};

  CAchievementRuntime runtime;
  runtime.SetLeaderboardState(state);

  ASSERT_TRUE(runtime.SetLeaderboardStanding(7, 12, "0:26.11", 1484));

  const LeaderboardInfo& updated = runtime.GetLeaderboardState().leaderboards.front();
  EXPECT_EQ(updated.playerRank, 12u);
  EXPECT_EQ(updated.playerScore, "0:26.11");
  EXPECT_EQ(updated.totalEntries, 1484u);

  // The fetched page predates the submission, so it is dropped rather than
  // having the new entry guessed into it
  EXPECT_TRUE(updated.entries.empty());

  // A board belonging to a game that has since been unloaded
  EXPECT_FALSE(runtime.SetLeaderboardStanding(999, 1, "0:01.00", 2));
}

TEST_F(TestLeaderboardUtils, ATotalOfZeroLeavesTheCountAlone)
{
  LeaderboardInfo leaderboard;
  leaderboard.id = 7;
  leaderboard.totalEntries = 1483;

  LeaderboardState state;
  state.leaderboards = {leaderboard};

  CAchievementRuntime runtime;
  runtime.SetLeaderboardState(state);

  // The server does not always say how many entries there are; keeping the
  // count is better than showing "of 0"
  ASSERT_TRUE(runtime.SetLeaderboardStanding(7, 12, "0:26.11", 0));

  EXPECT_EQ(runtime.GetLeaderboardState().leaderboards.front().totalEntries, 1483u);
}
