/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIHelpWizard.h"
#include "games/controllers/ControllerIDs.h"
#include "utils/log.h"



#include "games/controllers/dialogs/GUIDialogAxisDetection.h"
#include "games/controllers/guicontrols/GUIFeatureButton.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerFeature.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/interfaces/IButtonMapCallback.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/keyboard/KeymapActionMap.h"
#include "input/InputManager.h"
#include "input/IKeymap.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace GAME;

#define ESC_KEY_CODE  27
#define SKIPPING_DETECTION_MS  200

// Duration to wait for axes to neutralize after mapping is finished
#define POST_MAPPING_WAIT_TIME_MS  (5 * 1000)

CGUIHelpWizard::CGUIHelpWizard() :
  CThread("GUIHelpWizard")
{
}

CGUIHelpWizard::~CGUIHelpWizard() = default;

void CGUIHelpWizard::Run()
{
  Abort(true);

  // Reset synchronization variables
  m_inputEvent.Reset();

  Create();
}

bool CGUIHelpWizard::Abort(bool bWait /* = true */)
{
  bool bWasRunning = !m_bStop;

  StopThread(false);

  m_inputEvent.Set();

  if (bWait)
    StopThread(true);

  return bWasRunning;
}

void CGUIHelpWizard::Process()
{
  CLog::Log(LOGDEBUG, "GUIHelpWizard: Starting help wizard");

  InstallHooks();

  CLog::Log(LOGDEBUG, "GUIHelpWizard: Waiting for input");

  // TODO: Open dialog asynchronously

  // "Help"
  // <help text>
  //MESSAGING::HELPERS::ShowOKDialogText(CVariant{10043}, CVariant{35055});

  // Wait for input
  m_inputEvent.Wait();

  RemoveHooks();

  CLog::Log(LOGDEBUG, "GUIHelpWizard: Help wizard ended");
}

std::string CGUIHelpWizard::ControllerID() const
{
  return DEFAULT_CONTROLLER_ID;
}

bool CGUIHelpWizard::MapPrimitive(JOYSTICK::IButtonMap* buttonMap,
                                           IKeymap* keymap,
                                           const JOYSTICK::CDriverPrimitive& primitive)
{
  Abort(false);
  return true;
}

bool CGUIHelpWizard::OnKeyPress(const CKey& key)
{
  Abort(false);
  return true;
}

void CGUIHelpWizard::InstallHooks()
{
  // Install button mapper with lowest priority
  CServiceBroker::GetPeripherals().RegisterJoystickButtonMapper(this);

  // Install hook to reattach button mapper when peripherals change
  CServiceBroker::GetPeripherals().RegisterObserver(this);

  // Install hook to cancel the button mapper
  CServiceBroker::GetInputManager().RegisterKeyboardDriverHandler(this);
}

void CGUIHelpWizard::RemoveHooks()
{
  CServiceBroker::GetInputManager().UnregisterKeyboardDriverHandler(this);
  CServiceBroker::GetPeripherals().UnregisterObserver(this);
  CServiceBroker::GetPeripherals().UnregisterJoystickButtonMapper(this);
}

void CGUIHelpWizard::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessagePeripheralsChanged:
    {
      CServiceBroker::GetPeripherals().UnregisterJoystickButtonMapper(this);
      CServiceBroker::GetPeripherals().RegisterJoystickButtonMapper(this);
      break;
    }
    default:
      break;
  }
}
