/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "utils/Observer.h"

#include <string>

class CSetting;
class CSettings;

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 */
class CGameSettings : public ISettingCallback, public Observable
{
public:
  CGameSettings();
  ~CGameSettings() override;

  // General settings
  bool GamesEnabled();
  bool ShowOSDHelp();
  void SetShowOSDHelp(bool bShow);
  void ToggleGames();
  bool AutosaveEnabled();
  bool RewindEnabled();
  unsigned int MaxRewindTimeSec();
  std::string GetRAUsername() const;
  std::string GetRAToken() const;

  // Achievement state — updated by CCheevos when a game loads
  struct AchievementInfo
  {
    std::string title;
    std::string description;
    std::string badgeUrl;
    unsigned int points{0};
    bool earned{false};
  };

  struct AchievementState
  {
    std::string gameTitle;
    unsigned int gameId{0};
    unsigned int totalAchievements{0};
    unsigned int unlockedAchievements{0};
    std::string richPresence;
    std::vector<AchievementInfo> achievements;
    bool loaded{false};
  };

  void SetAchievementState(const AchievementState& state) { m_achievementState = state; }
  void ClearAchievementState() { m_achievementState = AchievementState{}; }
  AchievementState GetAchievementState() const { return m_achievementState; }

  // Inherited from ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

private:
  std::string LoginToRA(const std::string& username,
                        const std::string& password,
                        std::string token) const;
  bool IsAccountVerified(const std::string& username, const std::string& token) const;

  // Construction parameters
  std::shared_ptr<CSettings> m_settings;

  // Current achievement state
  AchievementState m_achievementState;
};

} // namespace GAME
} // namespace KODI
