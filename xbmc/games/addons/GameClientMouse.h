/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "input/mouse/IMouseInputHandler.h"

struct GameClient;

namespace GAME
{
  class CGameClient;

  class CGameClientMouse : public MOUSE::IMouseInputHandler
  {
  public:
    CGameClientMouse(const CGameClient* gameClient, const GameClient* dllStruct);
    ~CGameClientMouse();

    // implementation of IMouseInputHandler
    virtual std::string ControllerID(void) const override;
    virtual bool OnMotion(const std::string& relpointer, int dx, int dy) override;
    virtual bool OnButtonPress(const std::string& button) override;
    virtual void OnButtonRelease(const std::string& button) override;

  private:
    // Construction parameters
    const CGameClient* const m_gameClient;
    const GameClient* const m_dllStruct;
    const std::string m_controllerId;
  };
}
