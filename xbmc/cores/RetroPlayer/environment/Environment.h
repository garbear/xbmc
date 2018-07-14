/*
 *      Copyright (C) 2018 Team Kodi
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

#pragma once

namespace KODI
{
namespace GAME
{
  class CGameClient;
}

namespace RETRO
{
  class CEnvironment
  {
  public:
    CEnvironment(GAME::CGameClient &gameClient);
    ~CEnvironment();

    // implementation of IPlayback
    /*
    virtual void Initialize() override;
    virtual void Deinitialize() override;
    virtual bool CanPause() const override { return true; }
    virtual bool CanSeek() const override { return true; }
    virtual unsigned int GetTimeMs() const override { return m_playTimeMs; }
    virtual unsigned int GetTotalTimeMs() const override { return m_totalTimeMs; }
    virtual unsigned int GetCacheTimeMs() const override { return m_cacheTimeMs; }
    virtual void SeekTimeMs(unsigned int timeMs) override;
    virtual double GetSpeed() const override;
    virtual void SetSpeed(double speedFactor) override;
    virtual void PauseAsync() override;
    virtual std::string CreateSavestate() override;
    virtual bool LoadSavestate(const std::string& path) override;

    // implementation of IGameLoopCallback
    virtual void FrameEvent() override;
    virtual void RewindEvent() override;
    */

  private:
    /*
    void AddFrame();
    void RewindFrames(uint64_t frames);
    void AdvanceFrames(uint64_t frames);
    void UpdatePlaybackStats();
    void UpdateMemoryStream();
    */

    // Construction parameter
    GAME::CGameClient &m_gameClient;

    // Gameplay functionality
    //std::unique_ptr<IMemoryStream> m_memoryStream;
  };
}
}
