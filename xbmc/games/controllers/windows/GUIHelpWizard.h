/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IConfigurationWindow.h"
#include "input/joysticks/interfaces/IButtonMapper.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

namespace KODI
{
namespace GAME
{
  class CGUIHelpWizard : public IHelpWizard,
                         public JOYSTICK::IButtonMapper,
                         public KEYBOARD::IKeyboardDriverHandler,
                         public Observer,
                         protected CThread
  {
  public:
    CGUIHelpWizard();

    virtual ~CGUIHelpWizard();

    // implementation of IHelpWizard
    virtual void Run() override;
    virtual bool Abort(bool bWait = true) override;

    // implementation of IButtonMapper
    virtual std::string ControllerID() const override;
    virtual bool NeedsCooldown() const override { return true; }
    virtual bool AcceptsPrimitive(JOYSTICK::PRIMITIVE_TYPE type) const override { return true; }
    virtual bool MapPrimitive(JOYSTICK::IButtonMap* buttonMap,
                              IKeymap* keymap,
                              const JOYSTICK::CDriverPrimitive& primitive) override;
    virtual void OnEventFrame(const JOYSTICK::IButtonMap* buttonMap, bool bMotion) override { }
    virtual void OnLateAxis(const JOYSTICK::IButtonMap* buttonMap, unsigned int axisIndex) override { }

    // implementation of IKeyboardDriverHandler
    virtual bool OnKeyPress(const CKey& key) override;
    virtual void OnKeyRelease(const CKey& key) override { }

    // implementation of Observer
    virtual void Notify(const Observable& obs, const ObservableMessage msg) override;

  protected:
    // implementation of CThread
    virtual void Process() override;

  private:
    void InstallHooks();
    void RemoveHooks();

    // Synchronization events
    CEvent m_inputEvent;
  };
}
}
