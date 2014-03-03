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

#include "games/controllers/ControllerTypes.h"
#include "input/joysticks/IInputHandler.h"

namespace GAME
{
  class CGameClient;

  class CGameClientInput : public JOYSTICK::IInputHandler
  {
  public:
    CGameClientInput(CGameClient* addon, int port, const ControllerPtr& controller);

    // Implementation of IJoystickInputHandler
    virtual std::string ControllerID(void) const override;
    virtual bool HasFeature(const std::string& feature) const override;
    virtual JOYSTICK::INPUT_TYPE GetInputType(const std::string& feature) const override;
    virtual bool OnButtonPress(const std::string& feature, bool bPressed) override;
    virtual bool OnButtonMotion(const std::string& feature, float magnitude) override;
    virtual bool OnAnalogStickMotion(const std::string& feature, float x, float y) override;
    virtual bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override;

    //const ControllerPtr& Controller(void) const { return m_controller; }

  private:
    CGameClient* const  m_addon;
    const int           m_port;
    const ControllerPtr m_controller;
  };
}
