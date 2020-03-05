/*
 *      Copyright (C) 2017 Team Kodi
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

#include "cores/RetroPlayer/playback/IFramePlayback.h"
#include "cores/RetroPlayer/RetroPlayerClock.h"

#include <memory>
#include <stdint.h>

namespace KODI
{
namespace GAME
{
  class CGameClient;
}

namespace RETRO
{
  class CAction;
  class CGoal;
  class CReward;
  class CState;

  class CEnvironment : public IFramePlayback
  {
  public:
    CEnvironment(GAME::CGameClient &gameClient);

    ~CEnvironment() override;

    // Implementation of IFramePlayback
    uint64_t PastFramesAvailable() const override;
    uint64_t FutureFramesAvailable() const override;
    bool HasCurrentFrame() const override;
    uint64_t MaxFrameCount() const override;
    void SeekFrame(int64_t offsetFrames) override;
    void SetMaxFrameCount(uint64_t frameCount) override;
    void FrameEvent() override;

  private:
    // Environment functions
    static void UpdateState( uint64_t t,       CState& state, const CReward& reward, const CGoal& goal, const CAction& action);
    static void UpdateReward(uint64_t t, const CState& state,       CReward& reward, const CGoal& goal, const CAction& action);
    static void UpdateGoal(  uint64_t t, const CState& state, const CReward& reward,       CGoal& goal, const CAction& action);
    static void UpdateAction(uint64_t t, const CState& state, const CReward& reward, const CGoal& goal,       CAction& action);

    // Construction parameters
    GAME::CGameClient &m_gameClient;

    // Environment variables
    std::unique_ptr<CState> m_state;
    std::unique_ptr<CReward> m_reward;
    std::unique_ptr<CGoal> m_goal;
    std::unique_ptr<CAction> m_action;

    bool m_bStop = false;
    uint64_t m_timestamp = 0;
  };
}
}
