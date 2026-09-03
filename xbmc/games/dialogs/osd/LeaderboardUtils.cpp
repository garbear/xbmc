/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LeaderboardUtils.h"

#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "games/AchievementRuntime.h"
#include "profiles/ProfileManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <algorithm>
#include <cmath>

using namespace KODI::GAME;

namespace
{
//! RetroAchievements measures time trials in frames at 60Hz, whatever the
//! console actually ran at
constexpr double FRAMES_PER_SECOND = 60.0;

constexpr std::time_t SECONDS_PER_DAY = 24 * 60 * 60;

//! Rough, and deliberately so: these only decide which sentence to use
constexpr std::time_t DAYS_PER_MONTH = 30;
constexpr std::time_t DAYS_PER_YEAR = 365;
} // namespace

std::string KODI::GAME::FormatLeaderboardScore(unsigned int score, const std::string& format)
{
  if (format == "TIME" || format == "FRAMES")
  {
    const double totalSeconds = static_cast<double>(score) / FRAMES_PER_SECOND;
    const auto minutes = static_cast<unsigned int>(totalSeconds) / 60;
    const auto seconds = static_cast<unsigned int>(totalSeconds) % 60;
    const auto centiseconds =
        static_cast<unsigned int>((totalSeconds - std::floor(totalSeconds)) * 100);

    return StringUtils::Format("{}:{:02d}.{:02d}", minutes, seconds, centiseconds);
  }

  if (format == "TIMESECS")
    return StringUtils::Format("{}:{:02d}", score / 60, score % 60);

  if (format == "FIXED1")
    return StringUtils::Format("{:.1f}", static_cast<double>(score) / 10.0);
  if (format == "FIXED2")
    return StringUtils::Format("{:.2f}", static_cast<double>(score) / 100.0);
  if (format == "FIXED3")
    return StringUtils::Format("{:.3f}", static_cast<double>(score) / 1000.0);

  return std::to_string(score);
}

std::string KODI::GAME::FormatRelativeDate(std::time_t submitted, std::time_t now)
{
  if (submitted <= 0)
    return {};

  const auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  // A submission timestamped in the future is a clock disagreement rather than
  // anything meaningful, so it is treated as just now
  const std::time_t elapsed = (now > submitted) ? now - submitted : 0;
  const std::time_t days = elapsed / SECONDS_PER_DAY;

  if (days <= 0)
    return strings.Get(33006); // "Today"
  if (days == 1)
    return strings.Get(35346); // "Yesterday"
  if (days < DAYS_PER_MONTH)
    return StringUtils::Format(strings.Get(35347), static_cast<int>(days));

  if (days < DAYS_PER_YEAR)
  {
    const auto months = std::max(1, static_cast<int>(days / DAYS_PER_MONTH));
    if (months == 1)
      return strings.Get(35352); // "Last month"
    return StringUtils::Format(strings.Get(35348), months);
  }

  const auto years = std::max(1, static_cast<int>(days / DAYS_PER_YEAR));
  if (years == 1)
    return strings.Get(35353); // "Last year"
  return StringUtils::Format(strings.Get(35349), years);
}

std::string KODI::GAME::RankMedal(unsigned int rank)
{
  switch (rank)
  {
    case 1:
      return "gold";
    case 2:
      return "silver";
    case 3:
      return "bronze";
    default:
      return {};
  }
}

namespace
{
constexpr const char* CACHE_FILE = "gameleaderboards.xml";
constexpr const char* ROOT_ELEMENT = "gameleaderboards";
constexpr const char* BOARD_ELEMENT = "leaderboard";
constexpr const char* ENTRY_ELEMENT = "entry";

//! Standings older than this are refetched. A table that has stood for months
//! will not have moved overnight, but a week is long enough that somebody may
//! have beaten it.
constexpr std::time_t CACHE_MAX_AGE = 7 * 24 * 60 * 60;

//! Enough leaderboards to cover a session's browsing without the file growing
//! without bound
constexpr size_t MAX_CACHED_BOARDS = 200;

std::string CachePath()
{
  const auto settings = CServiceBroker::GetSettingsComponent();
  if (!settings)
    return {};

  const auto profileManager = settings->GetProfileManager();
  if (!profileManager)
    return {};

  return profileManager->GetUserDataItem(CACHE_FILE);
}
} // namespace

void KODI::GAME::SaveLeaderboardEntries(unsigned int leaderboardId,
                                        const std::vector<LeaderboardEntry>& entries)
{
  const std::string path = CachePath();
  if (path.empty() || entries.empty())
    return;

  CXBMCTinyXML2 doc;

  tinyxml2::XMLElement* root = nullptr;
  if (XFILE::CFile::Exists(path) && doc.LoadFile(path) && doc.RootElement() != nullptr &&
      std::string(doc.RootElement()->Value()) == ROOT_ELEMENT)
  {
    root = doc.RootElement();

    // Replace rather than accumulate: the same leaderboard looked at twice
    // should leave one record, not two
    for (auto* board = root->FirstChildElement(BOARD_ELEMENT); board != nullptr;)
    {
      auto* next = board->NextSiblingElement(BOARD_ELEMENT);
      if (board->UnsignedAttribute("id") == leaderboardId)
        root->DeleteChild(board);
      board = next;
    }
  }
  else
  {
    doc.Clear();
    root = doc.NewElement(ROOT_ELEMENT);
    doc.InsertEndChild(root);
  }

  if (root == nullptr)
    return;

  // Oldest first, so trimming takes the least recently looked at
  size_t boards = 0;
  for (auto* board = root->FirstChildElement(BOARD_ELEMENT); board != nullptr;
       board = board->NextSiblingElement(BOARD_ELEMENT))
    ++boards;

  while (boards >= MAX_CACHED_BOARDS)
  {
    auto* oldest = root->FirstChildElement(BOARD_ELEMENT);
    if (oldest == nullptr)
      break;
    root->DeleteChild(oldest);
    --boards;
  }

  auto* board = doc.NewElement(BOARD_ELEMENT);
  board->SetAttribute("id", leaderboardId);
  board->SetAttribute("fetched", static_cast<int64_t>(std::time(nullptr)));

  for (const LeaderboardEntry& entry : entries)
  {
    auto* element = doc.NewElement(ENTRY_ELEMENT);
    element->SetAttribute("rank", entry.rank);
    element->SetAttribute("user", entry.username.c_str());
    element->SetAttribute("score", entry.score.c_str());
    element->SetAttribute("submitted", static_cast<int64_t>(entry.submitted));
    element->SetAttribute("player", entry.isPlayer);
    board->InsertEndChild(element);
  }

  root->InsertEndChild(board);

  if (!doc.SaveFile(path))
    CLog::Log(LOGERROR, "Leaderboards: unable to save {}", path);
}

void KODI::GAME::ForgetLeaderboardEntries(unsigned int leaderboardId)
{
  const std::string path = CachePath();
  if (path.empty() || !XFILE::CFile::Exists(path))
    return;

  CXBMCTinyXML2 doc;
  if (!doc.LoadFile(path))
    return;

  auto* root = doc.RootElement();
  if (root == nullptr || std::string(root->Value()) != ROOT_ELEMENT)
    return;

  bool removed = false;
  for (auto* board = root->FirstChildElement(BOARD_ELEMENT); board != nullptr;)
  {
    auto* next = board->NextSiblingElement(BOARD_ELEMENT);
    if (board->UnsignedAttribute("id") == leaderboardId)
    {
      root->DeleteChild(board);
      removed = true;
    }
    board = next;
  }

  if (removed && !doc.SaveFile(path))
    CLog::Log(LOGERROR, "Leaderboards: unable to save {}", path);
}

void KODI::GAME::ClearLeaderboardEntries()
{
  const std::string path = CachePath();
  if (path.empty() || !XFILE::CFile::Exists(path))
    return;

  if (!XFILE::CFile::Delete(path))
    CLog::Log(LOGERROR, "Leaderboards: unable to delete {}", path);
  else
    CLog::Log(LOGINFO, "Leaderboards: kept standings cleared");
}

bool KODI::GAME::LoadLeaderboardEntries(unsigned int leaderboardId,
                                        std::vector<LeaderboardEntry>& entries)
{
  const std::string path = CachePath();
  if (path.empty() || !XFILE::CFile::Exists(path))
    return false;

  CXBMCTinyXML2 doc;
  if (!doc.LoadFile(path))
    return false;

  const auto* root = doc.RootElement();
  if (root == nullptr || std::string(root->Value()) != ROOT_ELEMENT)
    return false;

  for (const auto* board = root->FirstChildElement(BOARD_ELEMENT); board != nullptr;
       board = board->NextSiblingElement(BOARD_ELEMENT))
  {
    if (board->UnsignedAttribute("id") != leaderboardId)
      continue;

    int64_t fetched = 0;
    board->QueryInt64Attribute("fetched", &fetched);
    if (fetched <= 0 || std::time(nullptr) - static_cast<std::time_t>(fetched) > CACHE_MAX_AGE)
      return false;

    for (const auto* element = board->FirstChildElement(ENTRY_ELEMENT); element != nullptr;
         element = element->NextSiblingElement(ENTRY_ELEMENT))
    {
      LeaderboardEntry entry;
      entry.rank = element->UnsignedAttribute("rank");

      const char* user = element->Attribute("user");
      entry.username = (user != nullptr) ? user : "";

      const char* score = element->Attribute("score");
      entry.score = (score != nullptr) ? score : "";

      int64_t submitted = 0;
      element->QueryInt64Attribute("submitted", &submitted);
      entry.submitted = static_cast<std::time_t>(submitted);

      entry.isPlayer = element->BoolAttribute("player");

      entries.push_back(std::move(entry));
    }

    return !entries.empty();
  }

  return false;
}
