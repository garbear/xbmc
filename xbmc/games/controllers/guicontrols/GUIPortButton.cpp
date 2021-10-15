/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPortButton.h"

#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/windows/GUIPortDefines.h"
#include "guilib/LocalizeStrings.h"

using namespace KODI;
using namespace GAME;

CGUIPortButton::CGUIPortButton(const CGUIButtonControl& buttonControl,
                               const std::string& label,
                               ControllerPtr controller,
                               unsigned int index)
  : CGUIButtonControl(buttonControl), m_selectedController(std::move(controller))
{
  // Initialize CGUIButtonControl
  SetLabel(label);
  SetLabel2(GetLabel(m_selectedController));
  SetID(CONTROL_PORT_BUTTONS_START + index);
  SetVisible(true);
  AllocResources();
}

std::string CGUIPortButton::GetLabel(const ControllerPtr& controller)
{
  if (controller)
    return controller->Layout().Label();

  return g_localizeStrings.Get(13298); // "Disconnected"
}
