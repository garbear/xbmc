/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameAchievements.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr unsigned int TOAST_DISPLAY_TIME_MS = 6000;
constexpr unsigned int TOAST_MESSAGE_TIME_MS = 500;

// Thresholds of the rarity categories, as percentages of players who have
// earned the achievement
constexpr float RARITY_COMMON = 50.0f;
constexpr float RARITY_UNCOMMON = 10.0f;
constexpr float RARITY_RARE = 2.0f;

std::string Localize(uint32_t stringId)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(stringId);
}

/*!
 * \brief Translate a rarity percentage into a localized category
 *
 * \return The category, or an empty string if the rarity is unknown
 */
std::string RarityCategory(float rarity)
{
  if (rarity <= 0.0f)
    return {};

  if (rarity > RARITY_COMMON)
    return Localize(35290); // "Common"
  if (rarity > RARITY_UNCOMMON)
    return Localize(35291); // "Uncommon"
  if (rarity > RARITY_RARE)
    return Localize(35292); // "Rare"

  return Localize(35293); // "Ultra rare"
}

/*!
 * \brief Render a rarity percentage as one to four stars
 *
 * Kept apart from the category name so that the name stays translatable and
 * the stars stay out of the translated strings.
 *
 * \return The stars, or an empty string if the rarity is unknown
 */
std::string RarityStars(float rarity)
{
  if (rarity <= 0.0f)
    return {};

  if (rarity > RARITY_COMMON)
    return "★";
  if (rarity > RARITY_UNCOMMON)
    return "★★";
  if (rarity > RARITY_RARE)
    return "★★★";

  return "★★★★";
}
} // namespace

CDialogGameAchievements::CDialogGameAchievements()
  : CGUIDialog(WINDOW_DIALOG_GAME_ACHIEVEMENTS, "DialogGameControllers.xml"),
    m_items(std::make_unique<CFileItemList>()),
    m_viewControl(std::make_unique<CGUIViewControl>())
{
}

CDialogGameAchievements::~CDialogGameAchievements() = default;

void CDialogGameAchievements::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl->Reset();
  m_viewControl->SetParentWindow(GetID());
  m_viewControl->AddView(GetControl(CONTROL_CHEEVOS_LIST));
}

void CDialogGameAchievements::OnWindowUnload()
{
  m_viewControl->Reset();

  CGUIDialog::OnWindowUnload();
}

void CDialogGameAchievements::OnInitWindow()
{
  const CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  if (!gameSettings.GetAchievementsLoggedIn())
  {
    // "RetroAchievements", "Log in to RetroAchievements in Settings to use this feature."
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, Localize(35264),
                                          Localize(35285), TOAST_DISPLAY_TIME_MS, false,
                                          TOAST_MESSAGE_TIME_MS);
    Abort();
    return;
  }

  const AchievementState state = CServiceBroker::GetGameServices().AchievementRuntime().GetState();
  if (!state.loaded || state.achievements.empty())
  {
    // "RetroAchievements", "This game doesn't support RetroAchievements"
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, Localize(35264),
                                          Localize(35286), TOAST_DISPLAY_TIME_MS, false,
                                          TOAST_MESSAGE_TIME_MS);
    Abort();
    return;
  }

  // Populate before the base class runs, because it focuses the list as part
  // of initialization and cannot focus an empty one
  m_viewControl->SetCurrentView(DEFAULT_VIEW_LIST);
  RefreshList();

  CGUIDialog::OnInitWindow();
}

void CDialogGameAchievements::Abort()
{
  // CGUIDialog::Open() registers the dialog and makes it active before it
  // sends GUI_MSG_WINDOW_INIT, so by the time OnInitWindow() decides there is
  // nothing to show, the dialog is already on screen. A plain Close() then
  // queues the skin's 240ms WindowClose animation and returns with the dialog
  // still active, which renders as a flash.
  //
  // Forcing the close skips the animation and deinitializes synchronously, so
  // the dialog leaves the render list before the next frame is drawn.
  Close(true);
}

void CDialogGameAchievements::OnDeinitWindow(int nextWindowID)
{
  m_viewControl->Clear();
  m_items->Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CDialogGameAchievements::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_NOTIFY_ALL:
    {
      // Sent by the game add-on when an achievement is earned
      if (message.GetParam1() == GUI_MSG_REFRESH_LIST)
      {
        RefreshList();
        return true;
      }
      break;
    }
    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

void CDialogGameAchievements::RefreshList()
{
  const AchievementState state = CServiceBroker::GetGameServices().AchievementRuntime().GetState();

  // Preserve the selection across a refresh triggered by an unlock
  const int selectedItem = m_viewControl->GetSelectedItem();

  m_viewControl->Clear();
  m_items->Clear();

  std::vector<AchievementInfo> achievements = state.achievements;

  // Show earned achievements first, keeping the runtime's order within each
  // group so the list doesn't reshuffle as achievements are unlocked
  std::stable_sort(achievements.begin(), achievements.end(),
                   [](const AchievementInfo& lhs, const AchievementInfo& rhs)
                   { return lhs.earned && !rhs.earned; });

  uint64_t totalPoints = 0;
  uint64_t earnedPoints = 0;

  for (const AchievementInfo& achievement : achievements)
  {
    auto item = std::make_shared<CFileItem>(achievement.title);
    item->SetLabel(achievement.title);
    item->SetLabel2(achievement.description);

    // Show the greyscale badge for achievements the player hasn't earned, but
    // fall back to the full-colour one if the add-on didn't supply it
    const std::string& badgeUrl = (achievement.earned || achievement.lockedBadgeUrl.empty())
                                      ? achievement.badgeUrl
                                      : achievement.lockedBadgeUrl;
    if (!badgeUrl.empty())
      item->SetArt("icon", badgeUrl);

    // "{0:d} pts"
    item->SetProperty(ACHIEVEMENT_POINTS, StringUtils::Format(Localize(35294), achievement.points));
    // Only set when true: a CVariant holding false stringifies to "false",
    // which String.IsEmpty() in the skin reads as present
    if (achievement.earned)
      item->SetProperty(ACHIEVEMENT_EARNED, true);

    if (achievement.earned && achievement.unlockedDate.IsValid())
    {
      // "Unlocked {0:s}"
      item->SetProperty(ACHIEVEMENT_UNLOCKED_DATE,
                        StringUtils::Format(Localize(35289), achievement.unlockedDate.GetAsLocalizedDate(
                                                std::string{"MMM dd yyyy"})));
    }

    if (achievement.rarity > 0.0f)
    {
      item->SetProperty(ACHIEVEMENT_RARITY_CATEGORY, RarityCategory(achievement.rarity));
      item->SetProperty(ACHIEVEMENT_RARITY_STARS, RarityStars(achievement.rarity));
    }

    // Only achievements that count something have progress worth showing, and
    // an earned one is by definition finished
    const bool measured = !achievement.earned && !achievement.measuredProgress.empty();
    if (measured)
    {
      item->SetProperty(ACHIEVEMENT_MEASURED, true);
      item->SetProperty(ACHIEVEMENT_MEASURED_PROGRESS, achievement.measuredProgress);
    }

    // Set on every row, including the ones with no progress. A list layout
    // shares one progress control between all rows, and CGUIProgressControl
    // keeps its last percentage when an item doesn't resolve the info - so a
    // row without this property would briefly draw the previous row's bar.
    //
    // Stored as an integer because the control reads it via CVariant::asInteger()
    item->SetProperty(ACHIEVEMENT_MEASURED_PERCENT,
                      measured ? static_cast<int>(std::lround(
                                     std::clamp(achievement.measuredPercent, 0.0f, 100.0f)))
                               : 0);

    totalPoints += achievement.points;
    if (achievement.earned)
      earnedPoints += achievement.points;

    m_items->Add(std::move(item));
  }

  m_viewControl->SetItems(*m_items);

  // std::clamp() is undefined when the list is empty, since the upper bound
  // would fall below the lower one
  if (!m_items->IsEmpty())
    m_viewControl->SetSelectedItem(std::clamp(selectedItem, 0, m_items->Size() - 1));

  // Progress weighted by point value, which is how RetroAchievements measures
  // completion. Guard against a game whose achievements are all worth 0 points.
  std::string progress;
  if (totalPoints > 0)
  {
    const int percent = static_cast<int>((earnedPoints * 100) / totalPoints);

    // "{0:d}% complete"
    progress = StringUtils::Format(Localize(35288), percent);
  }

  // Build the header here rather than in the skin, so that skins don't have to
  // reproduce the punctuation and the empty cases
  //
  // "Achievements - Chrono Trigger (74% complete)"
  std::string header = Localize(35287);
  if (!state.gameTitle.empty())
    header += " - " + state.gameTitle;
  if (!progress.empty())
    header += " (" + progress + ")";

  SetProperty("Header", header);
}
