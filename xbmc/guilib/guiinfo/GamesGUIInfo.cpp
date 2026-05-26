/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/GamesGUIInfo.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/addons/GameClient.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/GUIComponent.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "settings/MediaSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace KODI::GUILIB::GUIINFO;
using namespace KODI::GAME;
using namespace KODI::RETRO;

namespace
{
const CGameInfoTag* GetGUIGameTag()
{
  if (const auto* gui = CServiceBroker::GetGUIConst(); gui != nullptr)
    return gui->GetInfoManager().GetCurrentGameTag();
  return nullptr;
}
} // namespace

bool CGamesGUIInfo::InitCurrentItem(CFileItem* item)
{
  if (item && item->IsGame())
  {
    CLog::Log(LOGDEBUG, "CGamesGUIInfo::InitCurrentItem({})", item->GetPath());
    item->LoadGameTag();
    CGameInfoTag* tag = item->GetGameInfoTag();
    if (tag->GetTitle().empty())
    {
      std::string title = CUtil::GetTitleFromPath(item->GetPath(), item->IsFolder());
      if (!title.empty())
        tag->SetTitle(title);
    }
    if (tag->GetPlatform().empty())
    {
      if (const std::string& gameClient = tag->GetGameClient(); !gameClient.empty())
      {
        ADDON::AddonPtr addon;
        if (CServiceBroker::GetAddonMgr().GetAddon(gameClient, addon, ADDON::AddonType::GAMEDLL,
                                                   ADDON::OnlyEnabled::CHOICE_YES))
        {
          const std::string& platforms =
              std::static_pointer_cast<CGameClient>(addon)->GetPlatforms();
          if (!platforms.empty())
            tag->SetPlatform(platforms);
        }
      }
    }
    return true;
  }
  return false;
}

bool CGamesGUIInfo::GetLabel(std::string& value,
                             const CFileItem* item,
                             int contextWindow,
                             const CGUIInfo& info,
                             std::string* fallback) const
{
  switch (info.GetInfo())
  {
    case RETROPLAYER_VIDEO_FILTER:
    {
      value = CMediaSettings::GetInstance().GetCurrentGameSettings().VideoFilter();
      return true;
    }
    case RETROPLAYER_STRETCH_MODE:
    {
      STRETCHMODE stretchMode =
          CMediaSettings::GetInstance().GetCurrentGameSettings().StretchMode();
      value = CRetroPlayerUtils::StretchModeToIdentifier(stretchMode);
      return true;
    }
    case RETROPLAYER_VIDEO_ROTATION:
    {
      const unsigned int rotationDegCCW =
          CMediaSettings::GetInstance().GetCurrentGameSettings().RotationDegCCW();
      value = std::to_string(rotationDegCCW);
      return true;
    }
    case RETROPLAYER_TITLE:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetTitle();
        return true;
      }
      break;
    }
    case RETROPLAYER_PLATFORM:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetPlatform();
        return true;
      }
      break;
    }
    case RETROPLAYER_GENRES:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = StringUtils::Join(tag->GetGenres(), ", ");
        return true;
      }
      break;
    }
    case RETROPLAYER_PUBLISHER:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetPublisher();
        return true;
      }
      break;
    }
    case RETROPLAYER_DEVELOPER:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetDeveloper();
        return true;
      }
      break;
    }
    case RETROPLAYER_OVERVIEW:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetOverview();
        return true;
      }
      break;
    }
    case RETROPLAYER_GAME_CLIENT:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetGameClient();
        return true;
      }
      break;
    }
    case RETROPLAYER_DISC_LABEL:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (appPlayer)
        value = appPlayer->DiscLabel();
      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_GAME_TITLE:
    {
      value = CServiceBroker::GetGameServices().GameSettings().GetAchievementGameTitle();
      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_TOTAL:
    {
      value =
          std::to_string(CServiceBroker::GetGameServices().GameSettings().GetAchievementTotal());
      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_UNLOCKED:
    {
      value =
          std::to_string(CServiceBroker::GetGameServices().GameSettings().GetAchievementUnlocked());
      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_STATUS:
    {
      const auto& gs = CServiceBroker::GetGameServices().GameSettings();
                if (!gs.GetAchievementsLoggedIn() || !gs.GetAchievementsLoaded())
                  value = "N/A";
                else
                  value = std::to_string(gs.GetAchievementUnlocked()) + " / " +
                          std::to_string(gs.GetAchievementTotal());
                return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_RICH_PRESENCE:
    {
      value = CServiceBroker::GetGameServices().GameSettings().GetAchievementRichPresence();
      return true;
    }
    case RETROPLAYER_ACHIEVEMENT_TITLE:
    {
      const auto state = CServiceBroker::GetGameServices().GameSettings().GetAchievementState();
      const int idx = info.GetData2();
      if (idx >= 0 && idx < static_cast<int>(state.achievements.size()))
        value = state.achievements[idx].title;
      return true;
    }
    case RETROPLAYER_ACHIEVEMENT_DESCRIPTION:
    {
      const auto state = CServiceBroker::GetGameServices().GameSettings().GetAchievementState();
      const int idx = info.GetData2();
      if (idx >= 0 && idx < static_cast<int>(state.achievements.size()))
        value = state.achievements[idx].description;
      return true;
    }
    case RETROPLAYER_ACHIEVEMENT_BADGE_URL:
    {
      const auto state = CServiceBroker::GetGameServices().GameSettings().GetAchievementState();
      const int idx = info.GetData2();
      if (idx >= 0 && idx < static_cast<int>(state.achievements.size()))
        value = state.achievements[idx].badgeUrl;
      return true;
    }
    case RETROPLAYER_ACHIEVEMENT_POINTS:
    {
      const auto state = CServiceBroker::GetGameServices().GameSettings().GetAchievementState();
      const int idx = info.GetData2();
      if (idx >= 0 && idx < static_cast<int>(state.achievements.size()))
        value = std::to_string(state.achievements[idx].points);
      return true;
    }
    default:
      break;
  }
  return false;
}

bool CGamesGUIInfo::GetInt(int& value,
                           const CGUIListItem* gitem,
                           int contextWindow,
                           const CGUIInfo& info) const
{
  return false;
}

bool CGamesGUIInfo::GetBool(bool& value,
                            const CGUIListItem* gitem,
                            int contextWindow,
                            const CGUIInfo& info) const
{
  switch (info.GetInfo())
  {
    case RETROPLAYER_SUPPORTS_EJECT:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      value = appPlayer && appPlayer->SupportsDiscControl();
      return true;
    }
    case RETROPLAYER_DISC_EJECTED:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      value = appPlayer && appPlayer->IsDiscEjected();
      return true;
    }
    case RETROPLAYER_EMPTY_TRAY:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      value = appPlayer && appPlayer->IsTrayEmpty();
      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_LOADED:
    {
      value = CServiceBroker::GetGameServices().GameSettings().GetAchievementsLoaded();
      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_LOGGED_IN:
    {
      value = CServiceBroker::GetGameServices().GameSettings().GetAchievementsLoggedIn();
      return true;
    }
    case RETROPLAYER_ACHIEVEMENT_EARNED:
    {
      const auto state = CServiceBroker::GetGameServices().GameSettings().GetAchievementState();
      const int idx = info.GetData2();
      if (idx >= 0 && idx < static_cast<int>(state.achievements.size()))
        value = state.achievements[idx].earned;
      return true;
    }
    default:
      break;
  }
  return false;
}
