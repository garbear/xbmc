/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientCheevos.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "games/addons/GameClient.h"

using namespace KODI;
using namespace GAME;

constexpr int HASH_SIZE = 33;
constexpr int URL_SIZE = 512;
constexpr int RICH_PRESENCE_EVAL_SIZE = 512;
constexpr int POST_DATA_SIZE = 1024;

CGameClientCheevos::CGameClientCheevos(CGameClient& gameClient, AddonInstance_Game& addonStruct)
  : m_gameClient(gameClient), m_struct(addonStruct)
{
}

bool CGameClientCheevos::RCGenerateHashFromFile(std::string& hash,
                                                int consoleID,
                                                const std::string& filePath)
{
  char _hash[HASH_SIZE] = {};
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGenerateHashFromFile(
                              &m_struct, _hash, consoleID, filePath.c_str()),
                          "RCGenerateHashFromFile()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGenerateHashFromFile()");
  }

  if (error != GAME_ERROR_NO_ERROR)
    return false;

  hash = _hash;
  return true;
}

bool CGameClientCheevos::RCGetGameIDUrl(std::string& url, const std::string& hash)
{
  char _url[URL_SIZE] = {};
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->RCGetGameIDUrl(&m_struct, _url, URL_SIZE, hash.c_str()),
        "RCGetGameIDUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetGameIDUrl()");
  }

  if (error != GAME_ERROR_NO_ERROR)
    return false;

  url = _url;
  return true;
}

bool CGameClientCheevos::RCGetPatchFileUrl(std::string& url,
                                           const std::string& username,
                                           const std::string& token,
                                           unsigned gameID)
{
  char _url[URL_SIZE] = {};
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetPatchFileUrl(
                              &m_struct, _url, URL_SIZE, username.c_str(), token.c_str(), gameID),
                          "RCGetPatchFileUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetPatchFileUrl()");
  }

  if (error != GAME_ERROR_NO_ERROR)
    return false;

  url = _url;
  return true;
}

bool CGameClientCheevos::RCPostRichPresenceUrl(std::string& url,
                                               std::string& postData,
                                               const std::string& username,
                                               const std::string& token,
                                               unsigned gameID,
                                               const std::string& richPresence)
{
  char _url[URL_SIZE] = {};
  char _postData[POST_DATA_SIZE] = {};
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCPostRichPresenceUrl(
                              &m_struct, _url, URL_SIZE, _postData, POST_DATA_SIZE,
                              username.c_str(), token.c_str(), gameID, richPresence.c_str()),
                          "RCPostRichPresenceUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCPostRichPresenceUrl()");
  }

  if (error != GAME_ERROR_NO_ERROR)
    return false;

  url = _url;
  postData = _postData;
  return true;
}

void CGameClientCheevos::EnableRichPresence(const std::string& script)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->EnableRichPresence(&m_struct, script.c_str()),
                          "GetRichPresence()");
  }
  catch (...)
  {
    m_gameClient.LogException("GetRichPresence()");
  }
}

void CGameClientCheevos::GetRichPresenceEvaluation(std::string& evaluation)
{
  char _evaluation[RICH_PRESENCE_EVAL_SIZE] = {};
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->GetRichPresenceEvaluation(
                              &m_struct, _evaluation, RICH_PRESENCE_EVAL_SIZE),
                          "GetRichPresence()");

    evaluation = _evaluation;
  }
  catch (...)
  {
    m_gameClient.LogException("GetRichPresence()");
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
