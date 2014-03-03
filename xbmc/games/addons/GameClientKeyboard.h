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

#include "input/keyboard/IKeyboardHandler.h"

struct GameClient;

namespace GAME
{
  class CGameClient;

  class CGameClientKeyboard : public KEYBOARD::IKeyboardHandler
  {
  public:
    CGameClientKeyboard(const CGameClient* gameClient, const GameClient* dllStruct);
    ~CGameClientKeyboard();

    // implementation of IKeyboardHandler
    virtual bool OnKeyPress(const CKey& key) override;
    virtual void OnKeyRelease(const CKey& key) override;

  private:
    // Construction parameters
    const CGameClient* const m_gameClient;
    const GameClient* const m_dllStruct;
  };
}
