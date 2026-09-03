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

  /*!
   * \brief Whether achievements are earned in hardcore mode
   *
   * Hardcore doubles the points awarded, and requires the player to go without
   * rewind, save state loading, cheats and slow motion. RetroAchievements does
   * not allow a session started in casual mode to continue into hardcore, so
   * turning this on mid-game resets it.
   */
  bool GetAchievementsHardcore() const;

  /*!
   * \brief Turn hardcore mode on or off
   *
   * Used to drop back to casual when a session resumes from a save state,
   * which RetroAchievements requires.
   */
  void SetAchievementsHardcore(bool hardcore);

  /*!
   * \brief Whether achievements already earned can be triggered again
   */
  bool GetAchievementsEncore() const;

  /*!
   * \brief Whether to show the achievement being attempted over the game
   *
   * Some players want to know an attempt is live; others would rather nothing
   * covered the picture. It is on by default, because an indicator nobody asked
   * for is easier to turn off than one nobody knew existed.
   */
  bool GetChallengeIndicator() const;

  bool GetAchievementsLoggedIn() const;

  /*!
   * \brief Record whether the player is logged in to RetroAchievements
   *
   * Reported by the add-on, so a rejected token doesn't leave the UI claiming
   * the player is logged in.
   *
   * \param loggedIn True if the player is logged in
   */
  void SetAchievementsLoggedIn(bool loggedIn);

  // Inherited from ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

private:
  std::string LoginToRA(const std::string& username,
                        const std::string& password,
                        std::string token) const;
  bool IsAccountVerified(const std::string& username, const std::string& token) const;

  // Construction parameters
  std::shared_ptr<CSettings> m_settings;
};

} // namespace GAME
} // namespace KODI
