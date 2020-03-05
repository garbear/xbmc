/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/IPlayer.h"
#include "threads/CriticalSection.h"

namespace KODI
{
namespace RETRO
{
  class IPlayback;

  class CRetroEngine : public IPlayer
  {
  public:
    explicit CRetroEngine(IPlayerCallback& callback);
    ~CRetroEngine() override;

    // implementation of IPlayer
    bool OpenFile(const CFileItem& file, const CPlayerOptions& options) override;
    bool CloseFile(bool reopen = false) override;
    bool IsPlaying() const override;
    bool CanPause() override;
    void Pause() override;
    bool HasVideo() const override { return true; }
    bool HasAudio() const override { return true; }
    bool HasGame() const override { return true; }
    bool CanSeek() override;
    void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) override;
    void SeekPercentage(float fPercent = 0) override;
    float GetCachePercentage() override;
    void SetMute(bool bOnOff) override;
    void SeekTime(int64_t iTime = 0) override;
    bool SeekTimeRelative(int64_t iTime) override;
    void SetSpeed(float speed) override;
    bool OnAction(const CAction &action) override;
    std::string GetPlayerState() override;
    bool SetPlayerState(const std::string& state) override;
    void Render(bool clear, uint32_t alpha = 255, bool gui = true) override;
    bool IsRenderingVideo() override;
    bool HasGameAgent() override;

  private:
    // Synchronization parameters
    CCriticalSection m_engineMutex;
  };
}
}
