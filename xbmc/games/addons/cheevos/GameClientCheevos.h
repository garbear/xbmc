/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include <functional>
#include <string>

struct AddonInstance_Game;

namespace KODI
{
namespace RETRO
{
enum class RConsoleID;
}

namespace GAME
{

class CGameClient;

/*!
 * \ingroup games
 */
class CGameClientCheevos
{
public:
  CGameClientCheevos(CGameClient& gameClient, AddonInstance_Game& addonStruct);

  bool RCGenerateHashFromFile(std::string& hash,
                              RETRO::RConsoleID consoleID,
                              const std::string& filePath);
  bool RCGetGameIDUrl(std::string& url, const std::string& hash);
  bool RCGetPatchFileUrl(std::string& url,
                         const std::string& username,
                         const std::string& token,
                         unsigned int gameID);
  void SetRetroAchievementsCredentials(const std::string& username, const std::string& token);
  bool RCPostRichPresenceUrl(std::string& url,
                             std::string& postData,
                             const std::string& username,
                             const std::string& token,
                             unsigned gameID,
                             const std::string& richPresence);
  void RCEnableRichPresence(const std::string& script);
  void RCGetRichPresenceEvaluation(std::string& evaluation, RETRO::RConsoleID consoleID);

  void ActivateAchievement(unsigned int cheevoId, const std::string& memAddrExpression);
  void GetAchievementUrlId(const std::function<void(const std::string& achievementUrl,
                                                    unsigned int cheevoId)>& callback);

  // When the game is reset, the runtime should also be reset
  void RCResetRuntime();

  /*!
   * \brief Read a memory region from the loaded game.
   *
   * Wraps the game addon's GetMemory call. Used by the rc_client memory
   * callback to give rcheevos access to the emulated system's RAM.
   *
   * \param type  One of GAME_MEMORY_SYSTEM_RAM, GAME_MEMORY_SAVE_RAM, etc.
   * \param data  Set to a pointer into the core's memory (valid until UnloadGame).
   * \param size  Set to the size of the region in bytes.
   * \return true on success.
   */
  bool GetMemory(unsigned int type, uint8_t*& data, size_t& size);

private:
  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
};
} // namespace GAME
} // namespace KODI
