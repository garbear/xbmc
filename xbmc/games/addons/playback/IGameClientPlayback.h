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

#include <stdint.h>

namespace GAME
{
  class IGameClientPlayback
  {
  public:
    virtual ~IGameClientPlayback() = default;

    // Playback capabilities
    virtual bool CanPause() const = 0;
    virtual bool CanSeek() const = 0;

    // Basic playback
    virtual bool IsPaused() const = 0;
    virtual void PauseUnpause() = 0;

    // Reversible playback
    virtual unsigned int GetTimeMs() const = 0;
    virtual unsigned int GetTotalTimeMs() const = 0;
    virtual unsigned int GetCacheTimeMs() const = 0;
    virtual void SeekTimeMs(unsigned int timeMs) = 0;
    virtual void SetSpeed(float speedFactor) = 0;
  };

  class CGameClientDummyPlayback : public IGameClientPlayback
  {
  public:
    virtual ~CGameClientDummyPlayback() = default;

    // implementation of IGameClientPlayback
    virtual bool CanPause() const override { return false; }
    virtual bool CanSeek() const override { return false; }
    virtual bool IsPaused() const override { return false; }
    virtual void PauseUnpause() override { }
    virtual unsigned int GetTimeMs() const { return 0; }
    virtual unsigned int GetTotalTimeMs() const { return 0; }
    virtual unsigned int GetCacheTimeMs() const { return 0; }
    virtual void SeekTimeMs(unsigned int timeMs) { }
    virtual void SetSpeed(float speedFactor) { }
  };
}
