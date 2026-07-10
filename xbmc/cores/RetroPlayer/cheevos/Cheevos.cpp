/*
 *  Copyright (C) 2020-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information
 */

#include "Cheevos.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheevos/GameClientCheevos.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

// rcheevos
#define RC_CLIENT_SUPPORTS_HASH
#include "../rcheevos/include/rc_api_runtime.h"
#include "../rcheevos/include/rc_client.h"
#include "../rcheevos/src/rc_libretro.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

using namespace KODI;
using namespace RETRO;

thread_local CCheevos* CCheevos::s_initializingCheevos = nullptr;

namespace
{
constexpr const char* RA_USER_AGENT      = "Kodi RetroPlayer/1.0";
constexpr const char* RA_BASE_URL        = "https://retroachievements.org/dorequest.php";
constexpr const char* RA_GAME_ICON_CACHE = "special://profile/cache/retroachievements/icons/";
constexpr unsigned int TOAST_DISPLAY_MS      = 6000;
constexpr unsigned int TOAST_DISPLAY_LONG_MS = 8000;
constexpr unsigned int TOAST_MESSAGE_MS      = 500;
constexpr int RP_PING_INTERVAL_MS = 120000;
constexpr const char* SETTING_RA_USERNAME = "gamesachievements.username";
constexpr const char* SETTING_RA_TOKEN      = "gamesachievements.token";
constexpr const char* SETTING_RA_LOGGED_IN = "gamesachievements.loggedin";
constexpr unsigned int PSEUDO_ACH_ID_THRESHOLD = 1000000;
constexpr unsigned int RP_BUFFER_SIZE          = 512;
constexpr int RP_SLEEP_INTERVAL_MS             = 100;
} // namespace

// Constructor / Destructor

CCheevos::CCheevos(GAME::CGameClient* gameClient,
                   const std::string& userName,
                   const std::string& loginToken)
  : m_gameClient(gameClient), m_userName(userName), m_loginToken(loginToken)
{
  m_rcClient = rc_client_create(RcheevosReadMemory, RcheevosServerCall);
  if (m_rcClient == nullptr)
  {
    CLog::Log(LOGERROR, "CCheevos: rc_client_create failed");
    return;
  }
  rc_client_set_userdata(m_rcClient, this);
  rc_client_set_event_handler(m_rcClient, RcheevosEventHandler);
  rc_client_set_hardcore_enabled(m_rcClient, 0);

  // Enable verbose rc_client logging
  rc_client_enable_logging(m_rcClient, RC_CLIENT_LOG_LEVEL_VERBOSE,
    [](const char* message, const rc_client_t*) {
      CLog::Log(LOGDEBUG, "rc_client: {}", message);
    });

  // Enable verbose rc_libretro memory mapping logging
  rc_libretro_init_verbose_message_callback(
    [](const char* message) {
      CLog::Log(LOGDEBUG, "rc_libretro: {}", message);
    });

  CLog::Log(LOGDEBUG, "CCheevos: rc_client created");

  if (!m_userName.empty() && !m_loginToken.empty())
  {
    CLog::Log(LOGDEBUG, "CCheevos: logging in with saved token for \'{}\' ", m_userName);
    rc_client_begin_login_with_token(m_rcClient,
                                      m_userName.c_str(), m_loginToken.c_str(),
                                      RcheevosLoginCallback, nullptr);
  }
}

CCheevos::~CCheevos()
{
  m_richPresenceRunning = false;

  CServiceBroker::GetGameServices().GameSettings().ClearLeaderboardState();
  if (m_richPresenceThread.joinable())
    m_richPresenceThread.join();

  // Join background download threads before tearing down
  {
    std::lock_guard<std::mutex> lock(m_downloadThreadsMutex);
    for (auto& t : m_downloadThreads)
      if (t.joinable())
        t.join();
    m_downloadThreads.clear();
  }

  delete static_cast<rc_libretro_memory_regions_t*>(m_rcMemoryRegions);
  m_rcMemoryRegions = nullptr;

  if (m_rcClient != nullptr)
  {
    rc_client_destroy(m_rcClient);
    m_rcClient = nullptr;
  }
  CLog::Log(LOGDEBUG, "CCheevos::~CCheevos -- cleaned up");
}

// Public API

void CCheevos::EnableRichPresence()
{
  m_richPresenceRunning = false;
  if (m_richPresenceThread.joinable())
    m_richPresenceThread.join();

  if (m_rcClient != nullptr)
    rc_client_unload_game(m_rcClient);

  if (m_rcClient == nullptr)
    return;

  const std::string gamePath =
      CSpecialProtocol::TranslatePath(m_gameClient->GetGamePath());

  CLog::Log(LOGINFO, "CCheevos::EnableRichPresence -- loading \'{}\' ",
            URIUtils::GetFileName(gamePath));

  // Join any in-flight download threads before re-loading
  // then pre-initialise memory regions BEFORE rc_client begins loading.
  // rc_client validates achievement addresses during identify-and-load,
  // which happens BEFORE RcheevosGameLoadCallback fires. Memory must be
  // mapped now, or rc_client permanently marks all achievements invalid.
  {
    std::lock_guard<std::mutex> lock(m_downloadThreadsMutex);
    for (auto& t : m_downloadThreads)
      if (t.joinable())
        t.join();
    m_downloadThreads.clear();
  }

  delete static_cast<rc_libretro_memory_regions_t*>(m_rcMemoryRegions);
  auto* preRegions = new rc_libretro_memory_regions_t{};
  m_rcMemoryRegions = preRegions;
  s_initializingCheevos = this;
  rc_libretro_memory_init(preRegions, nullptr, RcheevosGetCoreMemoryInfo, RC_CONSOLE_UNKNOWN);
  s_initializingCheevos = nullptr;
  CLog::Log(LOGINFO, "CCheevos::EnableRichPresence -- memory pre-mapped, total_size={}",
            preRegions->total_size);

  rc_client_begin_identify_and_load_game(m_rcClient,
                                          RC_CONSOLE_UNKNOWN,
                                          gamePath.c_str(),
                                          nullptr, 0,
                                          RcheevosGameLoadCallback,
                                          nullptr);
}

void CCheevos::DoFrame()
{
  if (m_rcClient == nullptr)
    return;
  rc_client_do_frame(m_rcClient);
}

std::string CCheevos::GetRichPresenceEvaluation()
{
  if (m_rcClient == nullptr)
    return {};
  char buffer[RP_BUFFER_SIZE]{};
  rc_client_get_rich_presence_message(m_rcClient, buffer, sizeof(buffer));
  return buffer;
}

bool CCheevos::RCLogin(const std::string& password)
{
  if (m_userName.empty() || password.empty())
  {
    CLog::Log(LOGERROR, "CCheevos::RCLogin -- username or password is empty");
    return false;
  }
  if (m_rcClient == nullptr)
    return false;

  CLog::Log(LOGDEBUG, "CCheevos::RCLogin -- logging in as \'{}\' ", m_userName);
  rc_client_begin_login_with_password(m_rcClient,
                                       m_userName.c_str(), password.c_str(),
                                       RcheevosLoginCallback, nullptr);
  return true;
}

// Memory callback

uint32_t CCheevos::RcheevosReadMemory(uint32_t address, uint8_t* buffer,
                                       uint32_t num_bytes, rc_client_t* client)
{
  const CCheevos* cheevos = static_cast<CCheevos*>(rc_client_get_userdata(client));
  if (cheevos == nullptr || num_bytes == 0 || cheevos->m_rcMemoryRegions == nullptr)
    return 0;

  // rc_libretro_memory_read handles console-specific address translation
  // (e.g. Genesis RAM at 0xFF0000, SNES mirrors, etc.)
  const auto* regions =
      static_cast<const rc_libretro_memory_regions_t*>(cheevos->m_rcMemoryRegions);
  return rc_libretro_memory_read(regions, address, buffer, num_bytes);
}

// HTTP server callback

void CCheevos::RcheevosServerCall(const rc_api_request_t* request,
                                   rc_client_server_callback_t callback,
                                   void* callback_data, rc_client_t* /*client*/)
{
  const std::string url      = (request->url != nullptr)       ? request->url       : "";
  const std::string postData = (request->post_data != nullptr)  ? request->post_data : "";

  std::thread([url, postData, callback, callback_data]()
  {
    XFILE::CCurlFile curl;
    curl.SetRequestHeader("User-Agent", RA_USER_AGENT);
    std::string body;
    bool ok;

    if (!postData.empty())
    {
      curl.SetMimeType("application/x-www-form-urlencoded");
      ok = curl.Post(url, postData, body);
    }
    else
    {
      ok = curl.Get(url, body);
    }

    rc_api_server_response_t resp{};
    if (ok && !body.empty())
    {
      resp.body             = body.c_str();
      resp.body_length      = body.size();
      resp.http_status_code = 200;
    }
    else
    {
      resp.body             = "";
      resp.body_length      = 0;
      resp.http_status_code = 500;
      CLog::Log(LOGWARNING, "CCheevos::RcheevosServerCall -- HTTP failed for {}", url);
    }
    callback(&resp, callback_data);
  }).detach(); // Must be detached — rc_client owns the callback lifecycle
}

// Event handler

void CCheevos::RcheevosEventHandler(const rc_client_event_t* event, rc_client_t* client)
{
  CCheevos* cheevos = static_cast<CCheevos*>(rc_client_get_userdata(client));
  if (cheevos == nullptr || event == nullptr)
    return;

  switch (event->type)
  {
    case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
    {
      const rc_client_achievement_t* ach = event->achievement;
      if (ach == nullptr)
        break;

      // Filter rc_client pseudo-achievements (server warnings, system notices).
      // Real RA achievements have IDs below 1,000,000 and non-zero points.
      if (ach->id >= PSEUDO_ACH_ID_THRESHOLD || ach->points == 0)
      {
        CLog::Log(LOGDEBUG, "CCheevos: ignoring pseudo-achievement id={} '{}'",
                  ach->id, ach->title != nullptr ? ach->title : "");
        break;
      }

      const std::string title    = (ach->title != nullptr)     ? ach->title     : "Achievement Unlocked!";
      const std::string badgeUrl = (ach->badge_url != nullptr) ? ach->badge_url : "";

      CLog::Log(LOGINFO, "CCheevos: achievement triggered: {} (id={} {}pts)",
                title, ach->id, ach->points);

      const std::string localBadge =
          std::string(RA_GAME_ICON_CACHE) + StringUtils::Format("badge_{}.png", ach->id);

      if (!badgeUrl.empty())
      {
        if (XFILE::CFile::Exists(localBadge))
        {
          CGUIDialogKaiToast::QueueNotification(localBadge, "Achievement Unlocked!", title,
                                                TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
        }
        else
        {
          const std::string urlCopy   = badgeUrl;
          const std::string titleCopy = title;
          {
            std::lock_guard<std::mutex> lock(cheevos->m_downloadThreadsMutex);
            cheevos->m_downloadThreads.emplace_back([urlCopy, localBadge, titleCopy]()
            {
              XFILE::CDirectory::Create(RA_GAME_ICON_CACHE);
              XFILE::CCurlFile curl;
              curl.SetRequestHeader("User-Agent", RA_USER_AGENT);
              std::string data;
              if (curl.Get(urlCopy, data) && !data.empty())
              {
                XFILE::CFile out;
                if (out.OpenForWrite(localBadge, true))
                {
                  const ssize_t written = out.Write(data.data(), static_cast<ssize_t>(data.size()));
                  out.Close();
                  if (written == static_cast<ssize_t>(data.size()))
                  {
                    CGUIDialogKaiToast::QueueNotification(localBadge,
                                                          "Achievement Unlocked!", titleCopy,
                                                          TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
                    return;
                  }
                  CLog::Log(LOGWARNING, "CCheevos: partial write for badge: {}", localBadge);
                }
              }
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                                    "Achievement Unlocked!", titleCopy,
                                                    TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
            });
          }
        }
      }
      else
      {
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                              "Achievement Unlocked!", title,
                                              TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
      }
      break;
    }
    case RC_CLIENT_EVENT_GAME_COMPLETED:
    {
      const rc_client_game_t* gi = rc_client_get_game_info(client);
      const std::string gameTitle =
          (gi != nullptr && gi->title != nullptr) ? gi->title : "";
      const std::string masteryIcon =
          StringUtils::Format("{}game_{}.png", RA_GAME_ICON_CACHE,
                              (gi != nullptr) ? gi->id : 0u);
      if (XFILE::CFile::Exists(masteryIcon))
        CGUIDialogKaiToast::QueueNotification(masteryIcon, "Mastered!", gameTitle,
                                              TOAST_DISPLAY_LONG_MS, false, TOAST_MESSAGE_MS);
      else
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                              "Mastered!", gameTitle,
                                              TOAST_DISPLAY_LONG_MS, false, TOAST_MESSAGE_MS);
      break;
    }
    case RC_CLIENT_EVENT_SERVER_ERROR:
    {
      CLog::Log(LOGWARNING, "CCheevos: server error from RetroAchievements");
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning,
                                            "RetroAchievements",
                                            "Server communication error",
                                            TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
      break;
    }
    case RC_CLIENT_EVENT_DISCONNECTED:
    {
      CLog::Log(LOGWARNING, "CCheevos: disconnected from RetroAchievements");
      break;
    }
    default:
      CLog::Log(LOGDEBUG, "CCheevos: unhandled rc_client event {}", event->type);
      break;
  }
}

// Login callback

void CCheevos::RcheevosLoginCallback(int result, const char* errorMessage,
                                      rc_client_t* client, void* /*userdata*/)
{
  CCheevos* cheevos = static_cast<CCheevos*>(rc_client_get_userdata(client));

  if (result != RC_OK)
  {
    CLog::Log(LOGWARNING, "CCheevos: login failed: {}",
              errorMessage != nullptr ? errorMessage : "unknown error");
    return;
  }

  const rc_client_user_t* user = rc_client_get_user_info(client);
  if (user == nullptr)
    return;

  CLog::Log(LOGINFO, "CCheevos: logged in as \'{}\' ({} points)",
            user->display_name, user->score);

  if (cheevos == nullptr)
    return;

  {
    std::lock_guard<std::mutex> lock(cheevos->m_credentialsMutex);
    cheevos->m_loginToken = (user->token != nullptr)    ? user->token    : "";
    cheevos->m_userName   = (user->username != nullptr) ? user->username : cheevos->m_userName;
  }

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings)
  {
    settings->SetString(SETTING_RA_USERNAME, cheevos->m_userName);
    settings->SetString(SETTING_RA_TOKEN,    cheevos->m_loginToken);
    settings->SetBool(SETTING_RA_LOGGED_IN, true);
    settings->Save();
  }
}

// Game load callback

void CCheevos::RcheevosGameLoadCallback(int result, const char* errorMessage,
                                         rc_client_t* client, void* /*userdata*/)
{
  CCheevos* cheevos = static_cast<CCheevos*>(rc_client_get_userdata(client));
  if (cheevos == nullptr)
    return;

  if (result != RC_OK)
  {
    CLog::Log(LOGINFO, "CCheevos: game load: {}",
              errorMessage != nullptr ? errorMessage : "unknown error");
    return;
  }

  const rc_client_game_t* gameInfo = rc_client_get_game_info(client);
  const uint32_t consoleId = (gameInfo != nullptr) ? gameInfo->console_id : RC_CONSOLE_UNKNOWN;
  CLog::Log(LOGINFO, "CCheevos: console_id={} game_id={}", consoleId,
            (gameInfo != nullptr) ? gameInfo->id : 0u);

  // Populate memory regions via rc_libretro.
  // rc_libretro_memory_init builds a console-aware address map so rc_client
  // can correctly translate e.g. Genesis 0xFF0000 addresses to the right RAM ptr.
  // We store the result via void* pimpl to keep rc_libretro.h out of Cheevos.h.
  delete static_cast<rc_libretro_memory_regions_t*>(cheevos->m_rcMemoryRegions);
  auto* regions = new rc_libretro_memory_regions_t{};
  cheevos->m_rcMemoryRegions = regions;

  s_initializingCheevos = cheevos;
  rc_libretro_memory_init(regions, nullptr, RcheevosGetCoreMemoryInfo, consoleId);
  s_initializingCheevos = nullptr;

  CLog::Log(LOGINFO, "CCheevos: memory map ready, total_size={}",
            regions->total_size);

  // Get game summary once — used for both notification and GameSettings state
  rc_client_user_game_summary_t summary{};
  rc_client_get_user_game_summary(client, &summary);

  // Show game-load notification
  if (gameInfo != nullptr)
  {
    const std::string gameTitle = (gameInfo->title != nullptr) ? gameInfo->title : "";

    CLog::Log(LOGINFO, "CCheevos: loaded \'{}\' ({}/{} achievements)",
              gameTitle, summary.num_unlocked_achievements, summary.num_core_achievements);

    // Toast format: title on header line, "X/Y Achievements" on body
    const std::string countMsg = StringUtils::Format(
        "{} / {} Achievements Unlocked",
        summary.num_unlocked_achievements, summary.num_core_achievements);

    const std::string iconPath =
        StringUtils::Format("{}game_{}.png", RA_GAME_ICON_CACHE, gameInfo->id);

    if (!XFILE::CFile::Exists(iconPath) && gameInfo->badge_name != nullptr)
    {
      const std::string iconUrl = StringUtils::Format(
          "https://media.retroachievements.org/Images/{}.png", gameInfo->badge_name);
      const std::string iconPathCopy = iconPath;
      const std::string titleCopy = gameTitle;
      const std::string countCopy = countMsg;
      {
        std::lock_guard<std::mutex> lock(cheevos->m_downloadThreadsMutex);
        cheevos->m_downloadThreads.emplace_back([iconUrl, iconPathCopy, titleCopy, countCopy]()
        {
          XFILE::CDirectory::Create(RA_GAME_ICON_CACHE);
          XFILE::CCurlFile curl;
          curl.SetRequestHeader("User-Agent", RA_USER_AGENT);
          std::string data;
          if (curl.Get(iconUrl, data) && !data.empty())
          {
            XFILE::CFile out;
            if (out.OpenForWrite(iconPathCopy, true))
            {
              const ssize_t written = out.Write(data.data(), static_cast<ssize_t>(data.size()));
              out.Close();
              if (written != static_cast<ssize_t>(data.size()))
                CLog::Log(LOGWARNING, "CCheevos: partial write for icon: {}", iconPathCopy);
            }
          }
          CGUIDialogKaiToast::QueueNotification(
              XFILE::CFile::Exists(iconPathCopy) ? iconPathCopy : "",
              titleCopy, countCopy, TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
        });
      }
    }
    else
    {
      CGUIDialogKaiToast::QueueNotification(
          XFILE::CFile::Exists(iconPath) ? iconPath : "",
          gameTitle, countMsg, TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
    }
  }

  // Populate GameSettings achievement state for OSD dialogs
  {
    GAME::CGameSettings::AchievementState achState;
    achState.gameId = (gameInfo != nullptr) ? gameInfo->id : 0;
    achState.gameTitle = (gameInfo != nullptr && gameInfo->title != nullptr) ? gameInfo->title : "";
    achState.totalAchievements = summary.num_core_achievements;
    achState.unlockedAchievements = summary.num_unlocked_achievements;
    achState.loaded = true;

    rc_client_achievement_list_t* achList = rc_client_create_achievement_list(
        client, RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE,
        RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_LOCK_STATE);
    if (achList != nullptr)
    {
      for (uint32_t b = 0; b < achList->num_buckets; ++b)
      {
        const auto& bucket = achList->buckets[b];
        for (uint32_t a = 0; a < bucket.num_achievements; ++a)
        {
          const rc_client_achievement_t* ach = bucket.achievements[a];
          GAME::CGameSettings::AchievementInfo info;
          info.title = (ach->title != nullptr) ? ach->title : "";
          info.description = (ach->description != nullptr) ? ach->description : "";
          info.points = ach->points;
          info.earned = (ach->unlocked != RC_CLIENT_ACHIEVEMENT_UNLOCKED_NONE);
          if (ach->badge_url != nullptr)
            info.badgeUrl = ach->badge_url;
          if (ach->badge_locked_url != nullptr)
            info.lockedBadgeUrl = ach->badge_locked_url;
          if (ach->rarity > 0.0f)
            info.rarity = StringUtils::Format("{:.1f}", ach->rarity);
          if (ach->unlock_time > 0)
          {
            struct tm tmBuf;
#ifdef TARGET_POSIX
            localtime_r(&ach->unlock_time, &tmBuf);
#else
            localtime_s(&tmBuf, &ach->unlock_time);
#endif
            char dateBuf[64];
            std::strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M", &tmBuf);
            info.unlockedDate = dateBuf;
          }
          achState.achievements.push_back(std::move(info));
        }
      }
      rc_client_destroy_achievement_list(achList);
    }

    CServiceBroker::GetGameServices().GameSettings().SetAchievementState(achState);
    CLog::Log(LOGINFO, "CCheevos: pushed {} achievements to GameSettings",
              achState.achievements.size());
  }

  // Populate GameSettings leaderboard state for OSD dialogs
  {
    GAME::CGameSettings::LeaderboardState lbState;
    lbState.gameTitle = (gameInfo != nullptr && gameInfo->title != nullptr) ? gameInfo->title : "";
    lbState.loaded = true;

    rc_client_leaderboard_list_t* lbList = rc_client_create_leaderboard_list(
        client, RC_CLIENT_LEADERBOARD_LIST_GROUPING_NONE);
    if (lbList != nullptr)
    {
      for (uint32_t b = 0; b < lbList->num_buckets; ++b)
      {
        const auto& bucket = lbList->buckets[b];
        for (uint32_t l = 0; l < bucket.num_leaderboards; ++l)
        {
          const rc_client_leaderboard_t* lb = bucket.leaderboards[l];
          GAME::CGameSettings::LeaderboardInfo lbInfo;
          lbInfo.id = lb->id;
          lbInfo.title = (lb->title != nullptr) ? lb->title : "";
          lbInfo.description = (lb->description != nullptr) ? lb->description : "";
          switch (lb->format)
          {
            case RC_CLIENT_LEADERBOARD_FORMAT_TIME:
              lbInfo.format = "TIME";
              break;
            case RC_CLIENT_LEADERBOARD_FORMAT_SCORE:
              lbInfo.format = "SCORE";
              break;
            case RC_CLIENT_LEADERBOARD_FORMAT_VALUE:
              lbInfo.format = "VALUE";
              break;
            default:
              lbInfo.format = "SCORE";
              break;
          }
          lbInfo.lowerIsBetter = (lb->lower_is_better != 0);
          lbState.leaderboards.push_back(std::move(lbInfo));
        }
      }
      rc_client_destroy_leaderboard_list(lbList);
    }

    CServiceBroker::GetGameServices().GameSettings().SetLeaderboardState(lbState);
    CLog::Log(LOGINFO, "CCheevos: pushed {} leaderboards to GameSettings",
              lbState.leaderboards.size());
  }

  // Start rich-presence ping thread
  cheevos->m_richPresenceRunning = true;
  cheevos->m_richPresenceThread = std::thread(&CCheevos::RichPresencePingThread, cheevos);
  CLog::Log(LOGINFO, "CCheevos: game session active");
}

// Memory-info callback

void CCheevos::RcheevosGetCoreMemoryInfo(uint32_t id,
                                          rc_libretro_core_memory_info_t* info)
{
  if (s_initializingCheevos == nullptr)
    return;

  // GAME_MEMORY enum values match libretro memory IDs exactly
  const GAME_MEMORY type = static_cast<GAME_MEMORY>(id & GAME_MEMORY_MASK);

  uint8_t* data = nullptr;
  size_t size   = 0;

  if (s_initializingCheevos->m_gameClient->Cheevos().GetMemory(type, data, size) &&
      data != nullptr && size > 0)
  {
    // Tell rc_libretro_memory_init about this region so it can build
    // the console-aware address map used by rc_libretro_memory_read()
    info->data = data;
    info->size = size;
    CLog::Log(LOGDEBUG, "CCheevos: memory region id={} size={}", id, size);
  }
}

// Rich-presence ping thread

void CCheevos::RichPresencePingThread()
{
  if (m_rcClient == nullptr)
    return;

  const rc_client_game_t* gameInfo = rc_client_get_game_info(m_rcClient);
  if (gameInfo == nullptr)
    return;

  const uint32_t gameId = gameInfo->id;

  // Snapshot credentials under lock to avoid races with
  // RcheevosLoginCallback updating m_userName/m_loginToken concurrently.
  std::string userName;
  std::string loginToken;
  {
    std::lock_guard<std::mutex> lock(m_credentialsMutex);
    userName   = m_userName;
    loginToken = m_loginToken;
  }

  CLog::Log(LOGINFO, "CCheevos::RichPresencePingThread -- started for game {}", gameId);

  while (m_richPresenceRunning)
  {
    for (int i = 0; i < (RP_PING_INTERVAL_MS / RP_SLEEP_INTERVAL_MS) && m_richPresenceRunning; ++i)
      std::this_thread::sleep_for(std::chrono::milliseconds(RP_SLEEP_INTERVAL_MS));

    if (!m_richPresenceRunning)
      break;

    const std::string rpMessage = GetRichPresenceEvaluation();

    const std::string pingUrl =
        std::string(RA_BASE_URL) +
        "?r=ping&u=" + CURL::Encode(userName) +
        "&t=" + CURL::Encode(loginToken) +
        "&g=" + std::to_string(gameId);

    CLog::Log(LOGDEBUG, "CCheevos::RichPresencePingThread -- posting: {}", rpMessage);

    XFILE::CCurlFile curl;
    curl.SetRequestHeader("User-Agent", RA_USER_AGENT);
    curl.SetMimeType("application/x-www-form-urlencoded");
    std::string response;
    if (curl.Post(pingUrl, "m=" + CURL::Encode(rpMessage), response))
      CLog::Log(LOGDEBUG, "CCheevos::RichPresencePingThread -- ping sent OK");
    else
      CLog::Log(LOGWARNING, "CCheevos::RichPresencePingThread -- ping failed");
  }

  CLog::Log(LOGINFO, "CCheevos::RichPresencePingThread -- stopped");
}
