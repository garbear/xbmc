/*
 *  Copyright (C) 2012-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Cheevos.h"

#include "FileItem.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "games/tags/GameInfoTag.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/JSONVariantParser.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/auto_buffer.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

// API JSON Field names
constexpr auto SUCCESS = "Success";
constexpr auto PATCH_DATA = "PatchData";
constexpr auto GAME_ID = "GameID";
constexpr auto RICH_PRESENCE = "RichPresencePatch";
constexpr auto GAME_TITLE = "Title";
constexpr auto PUBLISHER = "Publisher";
constexpr auto DEVELOPER = "Developer";
constexpr auto GENRE = "Genre";
constexpr auto CONSOLE_NAME = "ConsoleName";
constexpr auto ACHIEVEMENTS = "Achievements";
constexpr auto MEM_ADDR = "MemAddr";
constexpr auto CHEEVO_ID = "ID";
constexpr auto FLAGS = "Flags";
constexpr auto CHEEVO_TITLE = "Title";
//constexpr auto GAME_POINTS = "Points";
constexpr auto BADGE_NAME = "BadgeName";
    //constexpr auto RELEASED = "Released"; // TODO: Currently unused

constexpr int HASH_SIZE = 33;
constexpr int RESPORNSE_SIZE = 64;
constexpr int URL_SIZE = 512;
constexpr int POST_DATA_SIZE = 1024;

CCheevos::CCheevos(GAME::CGameClient* gameClient,
                   CFileItem& fileItem,
                   std::string userName,
                   std::string loginToken)
  : m_gameClient(gameClient),
    m_fileItem(fileItem),
    m_userName(std::move(userName)),
    m_loginToken(std::move(loginToken))
{
}

void CCheevos::ResetRuntime()
{
  m_gameClient->RCResetRuntime();
}

bool CCheevos::LoadData()
{
  if (m_userName.empty() || m_loginToken.empty())
    return false;

  const std::string extension = URIUtils::GetExtension(m_gameClient->GetGamePath());
  auto it = m_extensionToConsole.find(extension);

  if (it == m_extensionToConsole.end())
    return false;

  if (m_romHash.empty())
  {
    char hash[HASH_SIZE] = {};
    if (!m_gameClient->RCGenerateHashFromFile(hash, it->second,
                                              m_gameClient->GetGamePath().c_str()))
    {
      return false;
    }

    m_romHash = hash;
  }

  char requestURL[URL_SIZE];

  if (!m_gameClient->RCGetGameIDUrl(requestURL, URL_SIZE, m_romHash.c_str()))
    return false;

  XFILE::CFile response;
  response.CURLCreate(requestURL);
  response.CURLOpen(0);

  char responseStr[RESPORNSE_SIZE];
  response.ReadString(responseStr, RESPORNSE_SIZE);

  response.Close();

  CVariant data(CVariant::VariantTypeObject);
  CJSONVariantParser::Parse(responseStr, data);

  if (!data[SUCCESS].asBoolean())
    return false;

  m_gameID = data[GAME_ID].asUnsignedInteger32();

  // For some reason RetroAchievements returns Success = true when the hash isn't found
  if (m_gameID == 0)
    return false;

  if (!m_gameClient->RCGetPatchFileUrl(requestURL, URL_SIZE, m_userName.c_str(),
                                       m_loginToken.c_str(), m_gameID))
    return false;
 

  CURL curl(requestURL);
  XUTILS::auto_buffer patchData;
  response.LoadFile(curl, patchData);
  std::string strResponse(patchData.get(), patchData.size());
  CJSONVariantParser::Parse(strResponse, data);
  if (!data[SUCCESS].asBoolean())
    return false;

  m_richPresenceScript = data[PATCH_DATA][RICH_PRESENCE].asString();
  //CLog::Log(LOGDEBUG, "rich presence now");
  //CLog::Log(LOGDEBUG, data[PATCH_DATA][RICH_PRESENCE].asString().c_str());
  m_richPresenceLoaded = true;
  
 // m_AchievementString = data[PATCH_DATA][ACHIEVEMENTS][1].asString();
  //CLog::Log(LOGDEBUG, m_AchievementString.c_str());
  //CLog::Log(LOGDEBUG, "flags now");
  //m_gameClient->ActivateAchievement(data[PATCH_DATA][ACHIEVEMENTS][7][CHEEVO_ID].asUnsignedInteger(),
                                   // data[PATCH_DATA][ACHIEVEMENTS][7][MEM_ADDR].asString().c_str());
  int i;
  for (i = 0; i < data[PATCH_DATA][ACHIEVEMENTS].size(); i++)
  {
    if (data[PATCH_DATA][ACHIEVEMENTS][i][FLAGS]==3)
    {
      cheevoid_list.push_back(data[PATCH_DATA][ACHIEVEMENTS][i][CHEEVO_ID].asUnsignedInteger());
      cheevo_memaddr.push_back(data[PATCH_DATA][ACHIEVEMENTS][i][MEM_ADDR].asString().c_str());
      cheevo_title.push_back(data[PATCH_DATA][ACHIEVEMENTS][i][CHEEVO_TITLE].asString().c_str());
    }
  }
  
  GAME::CGameInfoTag& tag = *m_fileItem.GetGameInfoTag();

  tag.SetTitle(data[PATCH_DATA][GAME_TITLE].asString());
  tag.SetPublisher(data[PATCH_DATA][PUBLISHER].asString());
  tag.SetDeveloper(data[PATCH_DATA][DEVELOPER].asString());
  tag.SetGenres({data[PATCH_DATA][GENRE].asString()});
  tag.SetPlatform(data[PATCH_DATA][CONSOLE_NAME].asString());

  return true;
}

void CCheevos::EnableRichPresence()
{
  if (!m_richPresenceLoaded)
  {
    if (!LoadData())
    {
      CLog::Log(LOGERROR, "Cheevos: Couldn't load patch file");
      return;
    }
  }

  m_gameClient->EnableRichPresence(m_richPresenceScript.c_str());
  m_richPresenceScript.clear();
}

void CCheevos::ActivateAchievement()
{
  if (!m_richPresenceLoaded)
  {
    if (!LoadData())
    {
      CLog::Log(LOGERROR, "Cheevos: Couldn't load patch file");
      return;
    }
  }
  for (int i = 0; i < cheevoid_list.size(); i++)
  {
    m_gameClient->ActivateAchievement(cheevoid_list[i],cheevo_memaddr[i]);
  }
  
}
bool CCheevos::GetRichPresenceEvaluation(char* evaluation, size_t size)
{
  if (!m_richPresenceLoaded)
  {
    CLog::Log(LOGERROR, "Cheevos: Rich Presence script was not found");
    return false;
  }

  m_gameClient->GetRichPresenceEvaluation(evaluation, size);

  GAME::CGameInfoTag& tag = *m_fileItem.GetGameInfoTag();

  tag.SetCaption(evaluation);

  CFileItem* file = new CFileItem(m_fileItem);
  MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_SET_PLAYER_ITEM, -1, -1,
                                                          static_cast<void*>(file));

  char url[URL_SIZE];
  char postData[POST_DATA_SIZE];
  if (m_gameClient->RCPostRichPresenceUrl(url, URL_SIZE, postData, POST_DATA_SIZE,
                                          m_userName.c_str(), m_loginToken.c_str(), m_gameID,
                                          evaluation))
  {
    XFILE::CCurlFile curl;
    std::string res;
    curl.Post(url, postData, res);
  }

  return true;
}
/*
void CCheevos::ActivateAchievement(unsigned id, const char* memaddr)
{
    
}*/