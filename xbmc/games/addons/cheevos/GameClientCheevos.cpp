/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientCheevos.h"

#include "ServiceBroker.h"
#include "TextureCache.h"
#include "XBDateTime.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "cores/RetroPlayer/cheevos/RConsoleIDs.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/addons/GameClient.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <utility>
#include <vector>

namespace
{
// C ABI expects a raw callback + context pointer, so we bridge to std::function here
void __cdecl GetCheevoUrlIdCallback(const void* context,
                                    const char* achievementUrl,
                                    unsigned int cheevoId)
{
  if (context == nullptr)
    return;

  const auto* callback = static_cast<
      const std::function<void(const std::string& achievementUrl, unsigned int cheevoId)>*>(
      context);
  if (!(*callback))
    return;

  try
  {
    (*callback)(achievementUrl != nullptr ? std::string{achievementUrl} : std::string{}, cheevoId);
  }
  catch (...)
  {
    // Never allow exceptions to unwind through the C ABI callback boundary
  }
}

// The add-on is not trusted to send sane counts, so cap what we copy out of it.
// The largest sets published by RetroAchievements are an order of magnitude
// below these limits.
constexpr size_t MAX_ACHIEVEMENTS = 4096;

constexpr unsigned int TOAST_DISPLAY_TIME_MS = 6000;
constexpr unsigned int TOAST_MESSAGE_TIME_MS = 500;

std::string SafeString(const char* str)
{
  return str != nullptr ? std::string{str} : std::string{};
}

std::string Localize(uint32_t stringId)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(stringId);
}

/*!
 * \brief Ask the achievements dialog to rebuild itself
 *
 * Called from the add-on's thread, so the message is queued rather than sent.
 * The dialog ignores it when it isn't open.
 */
void NotifyDialogs()
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui == nullptr)
    return;

  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, WINDOW_DIALOG_GAME_ACHIEVEMENTS, 0, GUI_MSG_REFRESH_LIST);
  gui->GetWindowManager().SendThreadMessage(msg, WINDOW_DIALOG_GAME_ACHIEVEMENTS);
}

/*!
 * \brief Convert a Unix timestamp to a localized date, or empty if unset
 */
std::string LocalizedDate(int64_t unixTime)
{
  if (unixTime <= 0)
    return {};

  // The format must be a std::string. Passing a string literal picks the
  // GetAsLocalizedDate(bool) overload instead, because const char* -> bool is a
  // standard conversion and beats const char* -> std::string
  return CDateTime(static_cast<time_t>(unixTime)).GetAsLocalizedDate(std::string{"MMM dd yyyy"});
}
} // namespace

using namespace KODI;
using namespace GAME;

CGameClientCheevos::CGameClientCheevos(CGameClient& gameClient, AddonInstance_Game& addonStruct)
  : m_gameClient(gameClient),
    m_struct(addonStruct)
{
}

bool CGameClientCheevos::RCGenerateHashFromFile(std::string& hash,
                                                RETRO::RConsoleID consoleID,
                                                const std::string& filePath)
{
  char* _hash = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->RCGenerateHashFromFile(
            &m_struct, &_hash, static_cast<unsigned int>(consoleID), filePath.c_str()),
        "RCGenerateHashFromFile()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGenerateHashFromFile()");
  }

  if (_hash)
  {
    hash = _hash;
    m_struct.toAddon->FreeString(&m_struct, _hash);
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientCheevos::RCGetGameIDUrl(std::string& url, const std::string& hash)
{
  char* _url = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetGameIDUrl(&m_struct, &_url, hash.c_str()),
                          "RCGetGameIDUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetGameIDUrl()");
  }

  if (_url)
  {
    url = _url;
    m_struct.toAddon->FreeString(&m_struct, _url);
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientCheevos::RCGetPatchFileUrl(std::string& url,
                                           const std::string& username,
                                           const std::string& token,
                                           unsigned int gameID)
{
  char* _url = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetPatchFileUrl(
                              &m_struct, &_url, username.c_str(), token.c_str(), gameID),
                          "RCGetPatchFileUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetPatchFileUrl()");
  }

  if (_url)
  {
    url = _url;
    m_struct.toAddon->FreeString(&m_struct, _url);
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientCheevos::RCPostRichPresenceUrl(std::string& url,
                                               std::string& postData,
                                               const std::string& username,
                                               const std::string& token,
                                               unsigned gameID,
                                               const std::string& richPresence)
{
  char* _url = nullptr;
  char* _postData = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCPostRichPresenceUrl(
                              &m_struct, &_url, &_postData, username.c_str(), token.c_str(), gameID,
                              richPresence.c_str()),
                          "RCPostRichPresenceUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCPostRichPresenceUrl()");
  }

  if (_url)
  {
    url = _url;
    m_struct.toAddon->FreeString(&m_struct, _url);
  }
  if (_postData)
  {
    postData = _postData;
    m_struct.toAddon->FreeString(&m_struct, _postData);
  }

  return error == GAME_ERROR_NO_ERROR;
}

void CGameClientCheevos::SetRetroAchievementsCredentials(const std::string& username,
                                                         const std::string& token)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;
  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->SetRetroAchievementsCredentials(
                              &m_struct, username.c_str(), token.c_str()),
                          "SetRetroAchievementsCredentials()");
  }
  catch (...)
  {
    m_gameClient.LogException("SetRetroAchievementsCredentials()");
  }
}

void CGameClientCheevos::RCEnableRichPresence(const std::string& script)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCEnableRichPresence(&m_struct, script.c_str()),
                          "RCEnableRichPresence()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCEnableRichPresence()");
  }
}

void CGameClientCheevos::RCGetRichPresenceEvaluation(std::string& evaluation,
                                                     RETRO::RConsoleID consoleID)
{
  char* _evaluation = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetRichPresenceEvaluation(
                              &m_struct, &_evaluation, static_cast<unsigned int>(consoleID)),
                          "RCGetRichPresenceEvaluation()");

    if (_evaluation)
    {
      evaluation = _evaluation;
      m_struct.toAddon->FreeString(&m_struct, _evaluation);
    }
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetRichPresenceEvaluation()");
  }
}

void CGameClientCheevos::ActivateAchievement(unsigned int cheevoId,
                                             const std::string& memAddrExpression)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->ActivateAchievement(&m_struct, cheevoId,
                                                                        memAddrExpression.c_str()),
                          "ActivateAchievement()");
  }
  catch (...)
  {
    m_gameClient.LogException("ActivateAchievement()");
  }
}

void CGameClientCheevos::GetAchievementUrlId(
    const std::function<void(const std::string& achievementUrl, unsigned int cheevoId)>& callback)
{
  if (!callback)
    return;

  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->GetCheevoUrlId(&m_struct, GetCheevoUrlIdCallback, &callback),
        "GetCheevoUrlId()");
  }
  catch (...)
  {
    m_gameClient.LogException("GetCheevoUrlId()");
  }
}

void CGameClientCheevos::RCResetRuntime()
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCResetRuntime(&m_struct), "RCResetRuntime()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCResetRuntime()");
  }
}

void CGameClientCheevos::OnGameLoaded(const game_rc_game_loaded& data)
{
  const std::string gameTitle = SafeString(data.title);

  // A null array must come with a zero count, but don't trust the add-on to
  // keep that promise
  const size_t achievementCount = data.achievements != nullptr ? data.achievement_count : 0;

  if (achievementCount > MAX_ACHIEVEMENTS)
  {
    CLog::Log(LOGWARNING, "CGameClientCheevos: game {} reported {} achievements, truncating to {}",
              data.game_id, achievementCount, MAX_ACHIEVEMENTS);
  }

  AchievementState achievementState;
  achievementState.gameTitle = gameTitle;
  achievementState.gameId = data.game_id;
  achievementState.loaded = true;
  achievementState.achievements.reserve(std::min(achievementCount, MAX_ACHIEVEMENTS));

  for (size_t i = 0; i < std::min(achievementCount, MAX_ACHIEVEMENTS); ++i)
  {
    const game_rc_achievement& achievement = data.achievements[i];

    AchievementInfo info;
    info.id = achievement.id;
    info.title = SafeString(achievement.title);
    info.description = SafeString(achievement.description);
    info.badgeUrl = SafeString(achievement.badge_url);
    info.lockedBadgeUrl = SafeString(achievement.badge_locked_url);
    info.rarity = std::clamp(achievement.rarity, 0.0f, 100.0f);
    info.points = achievement.points;
    info.earned = achievement.unlock_state != GAME_RC_UNLOCK_STATE_LOCKED;
    if (info.earned)
      info.unlockedDate = LocalizedDate(achievement.unlock_time);

    if (info.earned)
      ++achievementState.unlockedAchievements;

    achievementState.achievements.emplace_back(std::move(info));
  }

  achievementState.totalAchievements =
      static_cast<unsigned int>(achievementState.achievements.size());

  CLog::Log(LOGINFO, "CGameClientCheevos: loaded game {} \"{}\" with {} achievements ({} earned)",
            data.game_id, gameTitle, achievementState.totalAchievements,
            achievementState.unlockedAchievements);

  CServiceBroker::GetGameServices().AchievementRuntime().SetState(achievementState);

  // Warm the texture cache with the unlocked badges of achievements that are
  // still locked, because those are the ones whose badge an unlock toast will
  // need. Without this the toast is queued before its image has been fetched,
  // and the notification window keeps drawing whatever texture it last had -
  // the game icon from the load notification - until the download lands.
  for (const AchievementInfo& info : achievementState.achievements)
  {
    if (!info.earned && !info.badgeUrl.empty())
      CServiceBroker::GetTextureCache()->BackgroundCacheImage(info.badgeUrl);
  }

  NotifyDialogs();

  // Games without achievements are common; announcing them adds nothing
  if (achievementState.totalAchievements == 0)
    return;

  // "{0:d} of {1:d} achievements unlocked"
  const std::string description = StringUtils::Format(
      Localize(35284), achievementState.unlockedAchievements, achievementState.totalAchievements);

  // Remote URLs are resolved by Kodi's texture cache, so no download is needed
  // here. An empty path falls back to the default notification icon.
  CGUIDialogKaiToast::QueueNotification(
      SafeString(data.icon_url), !gameTitle.empty() ? gameTitle : Localize(35264), description,
      TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnAchievementTriggered(const game_rc_achievement_triggered& data)
{
  const std::string title = SafeString(data.title);

  // The add-on reports the unlock as it happens and carries no timestamp, so
  // the date is "now". Formatted the same way as the dates that come back with
  // the achievement list, otherwise a freshly earned achievement would be
  // styled differently from one earned in an earlier session.
  const std::string unlockedDate = LocalizedDate(std::time(nullptr));

  bool newlyEarned = false;
  CServiceBroker::GetGameServices().AchievementRuntime().MarkEarned(data.id, unlockedDate,
                                                                    newlyEarned);

  // The runtime re-reports achievements that were already earned in an earlier
  // session, so only announce the ones that changed state
  if (!newlyEarned)
  {
    CLog::Log(LOGDEBUG, "CGameClientCheevos: achievement {} \"{}\" was already earned", data.id,
              title);
    return;
  }

  CLog::Log(LOGINFO, "CGameClientCheevos: earned achievement {} \"{}\" ({} points){}", data.id,
            title, data.points, data.hardcore ? " in hardcore mode" : "");

  NotifyDialogs();

  // "Achievement Unlocked". This is the one notification that plays a sound,
  // following the same path as every other Kodi notification sound.
  CGUIDialogKaiToast::QueueNotification(SafeString(data.badge_url), Localize(35281),
                                        !title.empty() ? title : SafeString(data.description),
                                        TOAST_DISPLAY_TIME_MS, true, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnGameCompleted(const std::string& title, bool hardcore)
{
  CLog::Log(LOGINFO, "CGameClientCheevos: {} \"{}\"", hardcore ? "mastered" : "completed", title);

  // "Game mastered" / "Game completed"
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                        Localize(hardcore ? 35282 : 35283), title,
                                        TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnAchievementProgress(const game_rc_achievement_progress* progress,
                                               unsigned int count)
{
  std::vector<AchievementProgress> updates;
  updates.reserve(count);

  for (unsigned int i = 0; i < count; ++i)
  {
    AchievementProgress update;
    update.id = progress[i].id;
    update.measuredPercent = progress[i].measured_percent;
    update.measuredProgress = SafeString(progress[i].measured_progress);

    updates.emplace_back(std::move(update));
  }

  const unsigned int applied =
      CServiceBroker::GetGameServices().AchievementRuntime().SetAchievementProgress(updates);
  if (applied != updates.size())
  {
    CLog::Log(LOGWARNING,
              "CGameClientCheevos: {} of {} progress updates were for achievements not in the "
              "loaded game",
              updates.size() - applied, updates.size());
  }

  // No dialog refresh here: progress is deliberately a snapshot taken when the
  // achievements dialog opens, so redrawing the list on every change would
  // both fight the user's scrolling and defeat the point
}

void CGameClientCheevos::OnServerError(const std::string& message, const std::string& api)
{
  CLog::Log(LOGERROR, "CGameClientCheevos: server error from {}: {}",
            !api.empty() ? api : "RetroAchievements", message);

  // "RetroAchievements"
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, Localize(35264), message,
                                        TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnConnectionChanged(bool connected)
{
  // The add-on reports the transition rather than each failed unlock, so this
  // is one notification per outage instead of one per achievement
  CLog::Log(LOGINFO, "CGameClientCheevos: {} RetroAchievements",
            connected ? "reconnected to" : "disconnected from");

  // "Unlocks will be submitted when the connection returns." /
  // "Pending unlocks have been submitted."
  CGUIDialogKaiToast::QueueNotification(
      connected ? CGUIDialogKaiToast::Info : CGUIDialogKaiToast::Warning, Localize(35264),
      Localize(connected ? 35297 : 35296), TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnRichPresenceUpdated(const std::string& evaluation)
{
  CServiceBroker::GetGameServices().AchievementRuntime().SetRichPresence(evaluation);
}

void CGameClientCheevos::OnLoginResult(const game_rc_login_result& data)
{
  if (data.success)
  {
    const std::string username = !SafeString(data.display_name).empty()
                                     ? SafeString(data.display_name)
                                     : SafeString(data.username);

    // Not announced to the player: the add-on signs in on every game load, and
    // the sign-in performed from Settings reports its own result
    CLog::Log(LOGINFO, "CGameClientCheevos: logged in as \"{}\" with {} points", username,
              data.points);
  }
  else
  {
    CLog::Log(LOGWARNING, "CGameClientCheevos: login failed: {}",
              !SafeString(data.error_message).empty() ? SafeString(data.error_message)
                                                      : "no reason given");
  }

  // Keep Kodi's logged-in state in step with the add-on, so that a rejected
  // token doesn't leave the UI claiming the player is logged in
  CServiceBroker::GetGameServices().GameSettings().SetAchievementsLoggedIn(data.success);
}
