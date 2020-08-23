/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameSettings.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/auto_buffer.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

namespace
{
const std::string SETTING_GAMES_ENABLE = "gamesgeneral.enable";
const std::string SETTING_GAMES_SHOW_OSD_HELP = "gamesgeneral.showosdhelp";
const std::string SETTING_GAMES_ENABLEAUTOSAVE = "gamesgeneral.enableautosave";
const std::string SETTING_GAMES_ENABLEREWIND = "gamesgeneral.enablerewind";
const std::string SETTING_GAMES_REWINDTIME = "gamesgeneral.rewindtime";
const std::string SETTING_GAMES_ACHIEVEMENTS_USERNAME = "gamesachievements.username";
const std::string SETTING_GAMES_ACHIEVEMENTS_PASSWORD = "gamesachievements.password";
const std::string SETTING_GAMES_ACHIEVEMENTS_TOKEN = "gamesachievements.token";
} // namespace

CGameSettings::CGameSettings()
{
  m_settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  m_settings->RegisterCallback(this, {SETTING_GAMES_ENABLEREWIND, SETTING_GAMES_REWINDTIME,
                                      SETTING_GAMES_ACHIEVEMENTS_USERNAME,
                                      SETTING_GAMES_ACHIEVEMENTS_PASSWORD});
}

CGameSettings::~CGameSettings()
{
  m_settings->UnregisterCallback(this);
}

bool CGameSettings::GamesEnabled()
{
  return m_settings->GetBool(SETTING_GAMES_ENABLE);
}

bool CGameSettings::ShowOSDHelp()
{
  return m_settings->GetBool(SETTING_GAMES_SHOW_OSD_HELP);
}

void CGameSettings::SetShowOSDHelp(bool bShow)
{
  if (m_settings->GetBool(SETTING_GAMES_SHOW_OSD_HELP) != bShow)
  {
    m_settings->SetBool(SETTING_GAMES_SHOW_OSD_HELP, bShow);

    //! @todo Asynchronous save
    m_settings->Save();
  }
}

void CGameSettings::ToggleGames()
{
  m_settings->ToggleBool(SETTING_GAMES_ENABLE);
}

bool CGameSettings::AutosaveEnabled()
{
  return m_settings->GetBool(SETTING_GAMES_ENABLEAUTOSAVE);
}

bool CGameSettings::RewindEnabled()
{
  return m_settings->GetBool(SETTING_GAMES_ENABLEREWIND);
}

unsigned int CGameSettings::MaxRewindTimeSec()
{
  int rewindTimeSec = m_settings->GetInt(SETTING_GAMES_REWINDTIME);

  return static_cast<unsigned int>(std::max(rewindTimeSec, 0));
}

std::string CGameSettings::RAUsername()
{
  return m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_USERNAME);
}

std::string CGameSettings::RAToken()
{
  return m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_TOKEN);
}

void CGameSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();

  if (settingId == SETTING_GAMES_ENABLEREWIND || settingId == SETTING_GAMES_REWINDTIME)
  {
    SetChanged();
    NotifyObservers(ObservableMessageSettingsChanged);
  }
  else if (settingId == SETTING_GAMES_ACHIEVEMENTS_USERNAME ||
           settingId == SETTING_GAMES_ACHIEVEMENTS_PASSWORD)
  {
    const std::string username = m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_USERNAME);
    const std::string password = m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_PASSWORD);

    if (!username.empty() && !password.empty())
    {
      XFILE::CFile request;
      const CURL loginUrl(StringUtils::Format(
          "http://retroachievements.org/dorequest.php?r=login&u=%s&p=%s", username, password));
      XUTILS::auto_buffer response;
      CVariant data(CVariant::VariantTypeObject);

      request.LoadFile(loginUrl, response);
      CJSONVariantParser::Parse(response.get(), data);

      if (data["Success"].asBoolean())
      {
        const std::string token = data["Token"].asString();
        m_settings->SetString(SETTING_GAMES_ACHIEVEMENTS_TOKEN, token);
      }
      else
      {
        m_settings->SetString(SETTING_GAMES_ACHIEVEMENTS_PASSWORD, "");
        m_settings->SetString(SETTING_GAMES_ACHIEVEMENTS_TOKEN, "");

        // "RetroAchievements", "Incorrect User/Password!"
        CServiceBroker::GetEventLog().AddWithNotification(
            EventPtr(new CNotificationEvent(35264, 35265, EventLevel::Error)));
      }
    }
  }
}
