/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameIndicators.h"

#include "ServiceBroker.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/ApplicationMessenger.h"

using namespace KODI::GAME;

CDialogGameIndicators::CDialogGameIndicators()
  : CGUIDialog(
        WINDOW_DIALOG_GAME_INDICATORS, "DialogGameControllers.xml", DialogModalityType::MODELESS)
{
  m_loadType = KEEP_IN_MEMORY;
}

void CDialogGameIndicators::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  // Closing is decided here, not where the runtime changed: this runs on the
  // GUI thread and can act at once, so a burst of starts and stops cannot race
  // itself.
  if (!AnythingToShow())
  {
    Close();
    return;
  }

  MarkDirtyRegion();

  CGUIDialog::Process(currentTime, dirtyregions);
}

bool CDialogGameIndicators::AnythingToShow()
{
  auto& gameServices = CServiceBroker::GetGameServices();
  const auto& runtime = gameServices.AchievementRuntime();

  // Asked for the same one the skin will draw, so an indicator it has nothing
  // to show for cannot hold an empty dialog open compositing the GUI layer

  // The challenge indicator is the one a player can turn off, so it only counts
  // towards keeping this open when they have left it on
  const bool challenge =
      gameServices.GameSettings().GetChallengeIndicator() && runtime.GetShownChallenge().id != 0;

  return challenge || runtime.GetProgressIndicator().id != 0 ||
         !runtime.GetShownLeaderboardTracker().display.empty();
}

void CDialogGameIndicators::Register()
{
  // One registration rather than a call at every site that changes an
  // indicator, so that one added later is drawn without its author having to
  // remember to ask for it
  CServiceBroker::GetGameServices().AchievementRuntime().SetIndicatorCallback([]() { Show(); });
}

void CDialogGameIndicators::Show()
{
  if (!AnythingToShow())
    return;

  // Only ever opens. Closing is the dialog's own business, above, which is what
  // keeps the two decisions from racing each other across threads.
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTIVATE_WINDOW,
                                             WINDOW_DIALOG_GAME_INDICATORS, 0);
}
