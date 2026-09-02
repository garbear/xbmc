/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include "utils/Variant.h"

#include <map>
#include <string>

namespace KODI::GAME
{
/*!
 * \ingroup games
 *
 * \brief The cheats found for the game being played
 *
 * One switch per cheat out of the game's cheat file. Built on the settings
 * dialog so the switches are the ones the rest of Kodi uses, and so no skin
 * has to know this dialog exists.
 *
 * The list is only reachable while the game has cheats, so an empty one is
 * never shown.
 */
class CDialogGameCheats : public CGUIDialogSettingsManualBase
{
public:
  CDialogGameCheats();
  ~CDialogGameCheats() override;

protected:
  // Implementation of CGUIDialogSettingsBase
  void SetupView() override;

  //! Nothing to save: a cheat is applied the moment it is switched on
  bool Save() override { return true; }

  /*!
   * \brief Give each switch the name the cheat file gave it
   *
   * Settings normally label themselves from a string ID, which cheats read
   * out of a file at runtime cannot have.
   */
  std::string GetSettingsLabel(const std::shared_ptr<ISetting>& setting) override;

  /*!
   * \brief Explain the cheat the player is on, where the file explains it
   *
   * Cheat files carry the explanation in long_desc, and put instructions in
   * the description itself often enough that a name too long for its row is
   * worth repeating here in full.
   */
  void SetDescription(const CVariant& label) override;

  // Implementation of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

  // Implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

private:
  //! The name each switch should carry, by setting id
  std::map<std::string, std::string> m_labels;

  //! What to say about a cheat while it is the one focused, by setting id
  std::map<std::string, std::string> m_descriptions;
};
} // namespace KODI::GAME
