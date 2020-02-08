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

#include "Environment.h"
#include "Action.h"
#include "Goal.h"
#include "Reward.h"
#include "State.h"

using namespace RETROPLAYER;

CEnvironment::CEnvironment() :
  m_state(new CState(this)),
  m_reward(new CReward(this)),
  m_goal(new CGoal(this)),
  m_action(new CAction(this))/*,
  m_relativeFrameSeek(0)*/
{
}

bool CEnvironment::Create()
{
  return CThread::Create();
}

void CEnvironment::Destroy()
{
  StopThread(true);
}

void CEnvironment::Process()
{
  CState& state = *m_state;
  CReward& reward = *m_reward;
  CGoal& goal = *m_goal;
  CAction& action = *m_action;

  uint64_t t = 0;

  while (!m_bStop)
  {
    UpdateState(t, state, reward, goal, action);
    if (m_bStop)
      break;

    UpdateReward(t, state, reward, goal, action);
    if (m_bStop)
      break;

    Save(t);
    if (m_bStop)
      break;

    UpdateGoal(t, state, reward, goal, action);
    if (m_bStop)
      break;

    UpdateAction(t, state, reward, goal, action);

    //! @todo Seek

    t++;
  }
}

void CEnvironment::UpdateState(uint64_t t, CState& state, const CReward& reward, const CGoal& goal, const CAction& action)
{
  state.Update(t, reward, goal, action);
}

void CEnvironment::UpdateReward(uint64_t t, const CState& state, CReward& reward, const CGoal& goal, const CAction& action)
{
  reward.Update(t, state, goal, action);
}

void CEnvironment::UpdateGoal(uint64_t t, const CState& state, const CReward& reward, CGoal& goal, const CAction& action)
{
  goal.Update(t, state, reward, action);
}

void CEnvironment::UpdateAction(uint64_t t, const CState& state, const CReward& reward, const CGoal& goal, CAction& action)
{
  action.Update(t, state, reward, goal);
}

void CEnvironment::Save(uint64_t timestamp)
{
  CState state;
  CReward reward;
  CGoal goal;
  CAction action;

  if (GetState(timestamp, state) &&
      GetReward(timestamp, reward) &&
      GetGoal(timestamp, goal) &&
      GetAction(timestamp, action))
  {
    m_database.AddState(state, reward, goal, action);
  }
}
