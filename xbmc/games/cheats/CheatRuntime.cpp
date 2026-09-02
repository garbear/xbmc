/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CheatRuntime.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI::GAME;

namespace
{
constexpr auto SETTING_GAMES_CHEATS_PATH = "gamesgeneral.cheatspath";
constexpr auto CHEAT_EXTENSION = ".cht";

//! \brief Look for a cheat file beside the folder, then one level inside it
//!
//! The libretro cheat database is published a folder per system, so the
//! setting can point at the database itself or at a folder of loose files.
CCheatPack FindCheats(const std::string& cheatsFolder, const std::string& fileName)
{
  const std::string direct = URIUtils::AddFileToFolder(cheatsFolder, fileName);
  if (XFILE::CFile::Exists(direct))
    return CCheatPack::Load(direct);

  CFileItemList systems;
  if (!XFILE::CDirectory::GetDirectory(cheatsFolder, systems, "", XFILE::DIR_FLAG_NO_FILE_DIRS))
    return {};

  // A database laid out one folder per system can hold the same game name
  // under more than one console, and nothing here says which is meant. Sending
  // another console's codes is worse than sending none.
  std::string match;
  for (int i = 0; i < systems.Size(); ++i)
  {
    const CFileItemPtr& system = systems[i];
    if (!system->IsFolder())
      continue;

    const std::string path = URIUtils::AddFileToFolder(system->GetPath(), fileName);
    if (!XFILE::CFile::Exists(path))
      continue;

    if (!match.empty())
    {
      CLog::Log(LOGDEBUG, "CCheatRuntime: \"{}\" is in more than one system, using none", fileName);
      return {};
    }

    match = path;
  }

  if (!match.empty())
    return CCheatPack::Load(match);

  return {};
}
} // namespace

void CCheatRuntime::Load(const std::string& gamePath)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_pack = CCheatPack();
  m_enabled.clear();

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string cheatsFolder = settings->GetString(SETTING_GAMES_CHEATS_PATH);
  if (cheatsFolder.empty() || gamePath.empty())
    return;

  // The cheat file is named after the game, which is how the libretro cheat
  // database is published
  std::string name = URIUtils::GetFileName(gamePath);
  URIUtils::RemoveExtension(name);

  m_pack = FindCheats(cheatsFolder, name + CHEAT_EXTENSION);
  if (m_pack.IsEmpty())
    return;

  m_enabled.reserve(m_pack.Cheats().size());
  for (const Cheat& cheat : m_pack.Cheats())
    m_enabled.push_back(cheat.enabled);

  CLog::Log(LOGINFO, "CCheatRuntime: {} cheat(s) for \"{}\"", m_pack.Cheats().size(), name);

  Apply();
}

void CCheatRuntime::Clear()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_gameClient != nullptr)
    m_gameClient->CheatReset();

  m_pack = CCheatPack();
  m_enabled.clear();
}

bool CCheatRuntime::HasCheats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_clientTakesCheats && !m_pack.IsEmpty();
}

std::vector<Cheat> CCheatRuntime::GetCheats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::vector<Cheat> cheats = m_pack.Cheats();
  for (size_t i = 0; i < cheats.size() && i < m_enabled.size(); ++i)
    cheats[i].enabled = m_enabled[i];

  return cheats;
}

void CCheatRuntime::SetEnabled(unsigned int index, bool enabled)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (index >= m_enabled.size())
    return;

  m_enabled[index] = enabled;
  Apply();
}

void CCheatRuntime::SetGameClient(CGameClient* gameClient)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_gameClient = gameClient;

  // Asked here rather than when a cheat is applied, which is too late to
  // decide whether to offer any. Resetting an untouched game changes nothing.
  m_clientTakesCheats = gameClient != nullptr && gameClient->CheatReset();
}

void CCheatRuntime::Apply()
{
  if (m_gameClient == nullptr)
    return;

  // A cheat is identified by the slot it was given, so the set is sent whole
  // rather than one code at a time: switching one off means the ones after it
  // would otherwise answer to the wrong index.
  m_gameClient->CheatReset();

  // Only the ones switched on are sent. Cores are not obliged to honour the
  // enabled flag and several ignore it outright, applying whatever they are
  // handed -- fceumm adds every code it is given -- so a cheat that is off has
  // to be left out rather than sent as disabled.
  const std::vector<Cheat>& cheats = m_pack.Cheats();
  unsigned int slot = 0;
  for (size_t i = 0; i < cheats.size() && i < m_enabled.size(); ++i)
  {
    if (m_enabled[i])
      m_gameClient->SetCheat(slot++, true, cheats[i].code);
  }
}
