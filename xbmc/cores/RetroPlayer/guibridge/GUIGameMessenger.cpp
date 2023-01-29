/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameMessenger.h"

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"

using namespace KODI;
using namespace RETRO;

CGUIGameMessenger::CGUIGameMessenger(CRPProcessInfo& processInfo)
  : m_guiComponent(processInfo.GetRenderContext().GUI())
{
}

void CGUIGameMessenger::RefreshSavestates(const std::string& savestatePath)
{
  if (m_guiComponent != nullptr)
  {
    CGUIMessage message(GUI_MSG_REFRESH_LIST, 0, 0);
    message.SetStringParam(savestatePath);

    // Notify the in-game savestate dialog
    m_guiComponent->GetWindowManager().SendThreadMessage(message, WINDOW_DIALOG_IN_GAME_SAVES);
  }
}
