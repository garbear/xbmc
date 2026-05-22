/*
 *  Copyright (C) 2020-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information
 *
 *  AUTH FIX: adds RCLogin() declaration and changes m_activatedCheevoMap
 *  from static to instance member.
 */

#pragma once

#include "RConsoleIDs.h"

#include <atomic>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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
  ~CCheevos();
  CCheevos(GAME::CGameClient* gameClient,
           const std::string& userName,
           const std::string& loginToken);

  void ResetRuntime();
  bool LoadData();
  void EnableRichPresence();
  std::string GetRichPresenceEvaluation();
  void ActivateAchievement();
  static void Callback_URL_ID(const char* achievementUrl, unsigned int cheevoId);
  void CheckTriggeredAchievement();

  /*!
   * \brief Perform the actual HTTP login exchange with RetroAchievements.
   *
   * Call this with the user's PASSWORD when they press the Login button.
   * On success the returned token is stored internally and persisted to
   * settings — the password is never stored.
   *
   * \param password  The user's account password (not a token).
   * \return true on successful login.
   */
  bool RCLogin(const std::string& password);

private:
  RConsoleID ConsoleID();

  GAME::CGameClient* m_gameClient;
  std::string m_userName;
  std::string m_loginToken;

  bool m_richPresenceLoaded{false};
  std::string m_richPresenceScript;

  // FIX: was static in the original — state leaked between game sessions.
  std::unordered_map<unsigned, std::vector<std::string>> m_activatedCheevoMap;
  std::string m_gameTitle;
  unsigned int m_gameId{0};

  // Static map so Callback_URL_ID (raw fn ptr, no capture) can look up titles
  static std::unordered_map<unsigned, std::pair<std::string, std::string>> s_cheevoTitles;
  static std::mutex s_cheevoTitlesMutex;
  // Rich presence periodic ping
  std::atomic<bool> m_richPresenceRunning{false};
  std::thread m_richPresenceThread;
  void RichPresencePingThread();
};

} // namespace RETRO
} // namespace KODI
