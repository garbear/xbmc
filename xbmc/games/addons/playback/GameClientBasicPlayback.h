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
#pragma once

#include "IGameClientPlayback.h"
#include "GameLoop.h"

namespace GAME
{
  class CGameClient;

  class CGameClientBasicPlayback : public CGameClientDummyPlayback,
                                   public IGameLoopCallback
  {
  public:
    CGameClientBasicPlayback(CGameClient* gameClient, double fps);

    virtual ~CGameClientBasicPlayback();

    // implementation of IGameClientPlayback via CGameClientDummyPlayback
    virtual bool CanPause() const override { return true; }
    virtual bool IsPaused() const override;
    virtual void PauseUnpause() override;

    // implementation of IGameLoopCallback
    virtual void FrameEvent() override;
    virtual void RewindEvent() override { }

  private:
    CGameClient* const m_gameClient;
    CGameLoop m_gameLoop;
  };
}
