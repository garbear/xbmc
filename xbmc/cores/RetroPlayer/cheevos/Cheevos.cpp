/*
 *  Copyright (C) 2012-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Cheevos.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "URL.h"
#include "utils/auto_buffer.h"
#include "utils/JSONVariantParser.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace RETRO;

// API JSON Field names
constexpr auto SUCCESS = "Success";
constexpr auto GAME_ID = "GameID";
constexpr auto PATCH_DATA = "PatchData";
constexpr auto RICH_PRESENCE = "RichPresencePatch";

constexpr int URL_SIZE = 512;

CCheevos::CCheevos(GAME::CGameClient* gameClient, const std::string userName, const std::string loginToken)
  : m_gameClient(gameClient),
    m_userName(userName),
    m_loginToken(loginToken)
{
}

bool CCheevos::LoadData()
{
  const std::string extension = URIUtils::GetExtension(m_gameClient->GetGamePath());
  auto it = m_extensionToConsole.find(extension);

  if (it == m_extensionToConsole.end())
    return false;

  if (m_romHash.empty())
  {

    char hash[33];
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

  char responseStr[50];
  response.ReadString(responseStr, 50);

  response.Close();

  CVariant data(CVariant::VariantTypeObject);
  CJSONVariantParser::Parse(responseStr, data);

  if (!data[SUCCESS].asBoolean())
    return false;

  m_gameID = data[GAME_ID].asUnsignedInteger32();

  // For some reason RetroAchievements returns Success = true when the hash isn't found
  if (m_gameID == 0)
    return false;

  if (!m_gameClient->RCGetPatchFileUrl(requestURL, URL_SIZE, m_userName.c_str(), m_loginToken.c_str(), m_gameID))
    return false;

  CURL curl(requestURL);
  XUTILS::auto_buffer patchData;
  response.LoadFile(curl, patchData);

  CJSONVariantParser::Parse(patchData.get(), data);

  if (!data[SUCCESS].asBoolean())
    return false;

  m_richPresenceScript = data[PATCH_DATA][RICH_PRESENCE].asString();
  m_richPresenceLoaded = true;

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

bool CCheevos::GetRichPresenceEvaluation(char* evaluation, size_t size)
{
  if (!m_richPresenceLoaded)
  {
    CLog::Log(LOGERROR, "Cheevos: Rich Presence script was not found");
    return false;
  }

  m_gameClient->GetRichPresenceEvaluation(evaluation, size);
  return true;
}
