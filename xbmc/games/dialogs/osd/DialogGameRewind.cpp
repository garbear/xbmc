/*
 *  Copyright (C) 2018-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameRewind.h"
//#include "cores/RetroPlayer/guibridge/GUIGamePlaybackHandle.h" //! @todo
#include "ServiceBroker.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"

using namespace KODI;
using namespace GAME;

CDialogGameRewind::CDialogGameRewind() : CGUIDialog(WINDOW_DIALOG_GAME_REWIND, "DialogSeekBar.xml")
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

bool CDialogGameRewind::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      // Don't init this dialog if we aren't playing a game
      if (!RegisterDialog())
        return false;

      break;
    }
    case GUI_MSG_WINDOW_DEINIT:
    {
      UnregisterDialog();

      break;
    }
    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CDialogGameRewind::RegisterDialog()
{
  //m_gamePlaybackHandle = CServiceBroker::GetGameRenderManager().RegisterPlaybackDialog();

  //return m_gamePlaybackHandle.get() != nullptr;

  return true;
}

void CDialogGameRewind::UnregisterDialog()
{
  m_gamePlaybackHandle.reset();
}
