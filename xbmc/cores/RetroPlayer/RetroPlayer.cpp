/*
 *      Copyright (C) 2012-2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RetroPlayer.h"
#include "RetroPlayerAudio.h"
#include "RetroPlayerVideo.h"
#include "addons/AddonManager.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "games/addons/playback/IGameClientPlayback.h"
#include "games/addons/GameClient.h"
#include "games/tags/GameInfoTag.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "FileItem.h"

using namespace GAME;

CRetroPlayer::CRetroPlayer(IPlayerCallback& callback) :
  IPlayer(callback),
  m_renderManager(m_clock, this),
  m_processInfo(CProcessInfo::CreateInstance())
{
  g_Windowing.Register(this);
}

CRetroPlayer::~CRetroPlayer()
{
  CloseFile();
}

bool CRetroPlayer::OpenFile(const CFileItem& file, const CPlayerOptions& options)
{
  CLog::Log(LOGINFO, "RetroPlayer: Opening: %s", file.GetPath().c_str());

  if (IsPlaying())
    CloseFile();

  PrintGameInfo(file);

  // Get the game client ID from the file properties
  std::string gameClientId = file.GetProperty(FILEITEM_PROPERTY_GAME_CLIENT).asString();

  // If the fileitem's add-on is a game client, fall back to that
  if (gameClientId.empty())
  {
    if (file.HasAddonInfo() && file.GetAddonInfo()->Type() == ADDON::ADDON_GAMEDLL)
      gameClientId = file.GetAddonInfo()->ID();
  }

  // Resolve ID to game client ptr
  if (!gameClientId.empty())
  {
    ADDON::AddonPtr addon;
    if (ADDON::CAddonMgr::GetInstance().GetAddon(gameClientId, addon, ADDON::ADDON_GAMEDLL))
      m_gameClient = std::dynamic_pointer_cast<GAME::CGameClient>(addon);
  }

  bool bSuccess = false;

  if (m_gameClient && m_gameClient->Initialize())
  {
    m_audio.reset(new CRetroPlayerAudio);
    m_video.reset(new CRetroPlayerVideo(m_clock, m_renderManager, *m_processInfo));
    bSuccess = m_gameClient->OpenFile(file, m_audio.get(), m_video.get());
  }

  if (bSuccess)
  {
    m_callback.OnPlayBackStarted();
  }
  else
  {
    m_gameClient.reset();
    m_audio.reset();
    m_video.reset();
  }

  return bSuccess;
}

bool CRetroPlayer::CloseFile(bool reopen /* = false */)
{
  CLog::Log(LOGDEBUG, "RetroPlayer: Closing file");

  if (m_gameClient)
  {
    m_gameClient->CloseFile();
    m_callback.OnPlayBackEnded();
  }

  m_audio.reset();
  m_video.reset();

  return true;
}

bool CRetroPlayer::IsPlaying() const
{
  if (m_gameClient)
    return m_gameClient->IsPlaying();
  return false;
}

bool CRetroPlayer::CanPause()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->CanPause();
  return false;
}

void CRetroPlayer::Pause()
{
  if (!CanPause())
    return;

  if (m_gameClient)
  {
    m_gameClient->GetPlayback()->PauseUnpause();

    if (IsPaused())
      m_callback.OnPlayBackPaused();
    else
      m_callback.OnPlayBackResumed();
  }
}

bool CRetroPlayer::IsPaused() const
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->IsPaused();
  return false;
}

bool CRetroPlayer::CanSeek()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->CanSeek();
  return false;
}

void CRetroPlayer::Seek(bool bPlus /* = true */,
                        bool bLargeStep /* = false */,
                        bool bChapterOverride /* = false */)
{
  if (!CanSeek())
    return;

  if (m_gameClient)
  {
    /* TODO
    if (bPlus)
    {
      if (bLargeStep)
        m_gameClient->GetPlayback()->BigSkipForward();
      else
        m_gameClient->GetPlayback()->SmallSkipForward();
    }
    else
    {
      if (bLargeStep)
        m_gameClient->GetPlayback()->BigSkipBackward();
      else
        m_gameClient->GetPlayback()->SmallSkipBackward();
    }
    */
  }
}

void CRetroPlayer::SeekPercentage(float fPercent /* = 0 */)
{
  if (!CanSeek())
    return;

  if (fPercent < 0.0f  )
    fPercent = 0.0f;
  else if (fPercent > 100.0f)
    fPercent = 100.0f;

  int64_t totalTime = GetTotalTime();
  if (totalTime != 0)
    SeekTime(static_cast<int64_t>(totalTime * fPercent / 100.0f));
}

float CRetroPlayer::GetPercentage()
{
  if (m_gameClient)
  {
    const float timeMs = m_gameClient->GetPlayback()->GetTimeMs();
    const float totalMs = m_gameClient->GetPlayback()->GetTotalTimeMs();

    if (totalMs != 0.0f)
      return timeMs / totalMs * 100.0f;
  }

  return 0.0f;
}

float CRetroPlayer::GetCachePercentage()
{
  if (m_gameClient)
  {
    const float cacheMs = m_gameClient->GetPlayback()->GetCacheTimeMs();
    const float totalMs = m_gameClient->GetPlayback()->GetTotalTimeMs();

    if (totalMs != 0.0f)
      return cacheMs / totalMs * 100.0f;
  }
  return 0.0f;
}

void CRetroPlayer::SeekTime(int64_t iTime /* = 0 */)
{
  if (!CanSeek())
    return;

  if (m_gameClient)
    m_gameClient->GetPlayback()->SeekTimeMs(iTime);
}

bool CRetroPlayer::SeekTimeRelative(int64_t iTime)
{
  if (!CanSeek())
    return false;

  SeekTime(GetTime() + iTime);

  return true;
}

int64_t CRetroPlayer::GetTime()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->GetTimeMs();
  return 0;
}

int64_t CRetroPlayer::GetTotalTime()
{
  if (m_gameClient)
    return m_gameClient->GetPlayback()->GetTotalTimeMs();
  return 0;
}

bool CRetroPlayer::GetStreamDetails(CStreamDetails &details)
{
  // TODO
  return false;
}

void CRetroPlayer::ToFFRW(int iSpeed /* = 0 */)
{
  if (m_gameClient)
    m_gameClient->GetPlayback()->SetSpeed(iSpeed);
}

void CRetroPlayer::PrintGameInfo(const CFileItem &file) const
{
  const CGameInfoTag *tag = file.GetGameInfoTag();
  if (tag)
  {
    CLog::Log(LOGDEBUG, "RetroPlayer: ---------------------------------------");
    CLog::Log(LOGDEBUG, "RetroPlayer: Game tag loaded");
    CLog::Log(LOGDEBUG, "RetroPlayer: URL: %s", tag->GetURL().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Platform: %s", tag->GetPlatform().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Title: %s", tag->GetTitle().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Game Code: %s", tag->GetID().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Region: %s", tag->GetRegion().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Publisher: %s", tag->GetPublisher().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Format: %s", tag->GetFormat().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: Cartridge Type: %s", tag->GetCartridgeType().c_str());
    CLog::Log(LOGDEBUG, "RetroPlayer: ---------------------------------------");
  }
}
