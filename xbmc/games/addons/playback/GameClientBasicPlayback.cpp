/*
 *      Copyright (C) 2016 Team Kodi
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

#include "GameClientBasicPlayback.h"
#include "games/addons/GameClient.h"

using namespace GAME;

CGameClientBasicPlayback::CGameClientBasicPlayback(CGameClient* gameClient, double fps) :
  m_gameClient(gameClient),
  m_gameLoop(this, fps)
{
  m_gameLoop.Start();
}

CGameClientBasicPlayback::~CGameClientBasicPlayback()
{
  m_gameLoop.Stop();
}

bool CGameClientBasicPlayback::IsPaused() const
{
  return m_gameLoop.GetSpeed() == 0.0;
}

void CGameClientBasicPlayback::PauseUnpause()
{
  if (IsPaused())
    m_gameLoop.SetSpeed(1.0);
  else
    m_gameLoop.SetSpeed(0.0);
}

void CGameClientBasicPlayback::FrameEvent()
{
  m_gameClient->RunFrame();
}
