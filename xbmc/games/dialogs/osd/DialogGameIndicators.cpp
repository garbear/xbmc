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
        WINDOW_DIALOG_GAME_INDICATORS, "DialogGameIndicators.xml", DialogModalityType::MODELESS)
{
  m_loadType = KEEP_IN_MEMORY;
}

void CDialogGameIndicators::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  // Closing is decided here rather than where the runtime changed, because this
  // runs on the GUI thread and can act at once. Deciding it from the game thread
  // meant posting a close and testing IsActive() before the post had been
  // served, so a burst of starts and stops raced itself and left the dialog up
  // several seconds after the last one ended.
  if (!AnythingToShow())
  {
    Close();
    return;
  }

  // Where the game has its own DRM plane the GUI layer is only composited when
  // something dirties it, and a label quietly changing its text is not reliably
  // enough. This is only reached while an indicator is up, so the rest of the
  // session still gets the saving that optimisation exists for.
  MarkDirtyRegion();

  CGUIDialog::Process(currentTime, dirtyregions);
}

bool CDialogGameIndicators::AnythingToShow()
{
  auto& gameServices = CServiceBroker::GetGameServices();
  const auto& runtime = gameServices.AchievementRuntime();

  // The challenge indicator is the one a player can turn off, so it only counts
  // towards keeping this open when they have left it on
  const bool challenge =
      gameServices.GameSettings().GetChallengeIndicator() && !runtime.GetChallenges().empty();

  return challenge || runtime.GetProgressIndicator().id != 0 ||
         !runtime.GetLeaderboardTrackers().empty();
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
