/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameCheats.h"

#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/cheats/CheatRuntime.h"
#include "guilib/GUIMacros.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/StringUtils.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace KODI::GAME;

namespace
{
//! Cuts a label to a width the dialog can show. Counted in characters and cut
//! on a character boundary: a byte count both cuts short text that happens to
//! be multibyte and can split a character into invalid UTF-8.
std::string TruncateCharacters(const std::string& text, size_t characters)
{
  size_t pos = 0;
  for (size_t seen = 0; seen < characters && pos < text.size(); ++seen)
  {
    // Step over the character's continuation bytes, which are 10xxxxxx and do
    // not begin one, the same way utf8_strlen counts them
    do
      ++pos;
    while (pos < text.size() && (static_cast<unsigned char>(text[pos]) & 0xC0) == 0x80);
  }

  return text.substr(0, pos);
}

constexpr auto SETTING_CHEAT_PREFIX = "cheat";

//! "Cheats"
constexpr int HEADING_CHEATS = 35322;

//! Cheat files are written by hand and a description is sometimes a sentence
//! of instructions rather than a name. Past this the text runs under the
//! switch, and 99% of the database is shorter than it anyway.
constexpr size_t MAX_LABEL_LENGTH = 55;

std::string SettingId(size_t index)
{
  return StringUtils::Format("{}{}", SETTING_CHEAT_PREFIX, index);
}
} // namespace

CDialogGameCheats::CDialogGameCheats()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_GAME_CHEATS, "DialogSettings.xml")
{
}

CDialogGameCheats::~CDialogGameCheats() = default;

void CDialogGameCheats::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(HEADING_CHEATS);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067); // "Close"
}

std::string CDialogGameCheats::GetSettingsLabel(const std::shared_ptr<ISetting>& setting)
{
  const auto label = m_labels.find(setting->GetId());
  if (label != m_labels.end())
    return label->second;

  return CGUIDialogSettingsManualBase::GetSettingsLabel(setting);
}

void CDialogGameCheats::SetDescription(const CVariant& label)
{
  const BaseSettingControlPtr control = GetSettingControl(m_iSetting);
  if (control != nullptr && control->GetSetting() != nullptr)
  {
    const auto description = m_descriptions.find(control->GetSetting()->GetId());
    if (description != m_descriptions.end())
    {
      CGUIDialogSettingsManualBase::SetDescription(description->second);
      return;
    }

    // A cheat the file said nothing more about: leave the area empty rather
    // than showing the heading the settings framework falls back to
    if (m_labels.find(control->GetSetting()->GetId()) != m_labels.end())
    {
      CGUIDialogSettingsManualBase::SetDescription(CVariant{""});
      return;
    }
  }

  CGUIDialogSettingsManualBase::SetDescription(label);
}

void CDialogGameCheats::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("gamecheats", HEADING_CHEATS);
  if (category == nullptr)
    return;

  const std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == nullptr)
    return;

  m_labels.clear();
  m_descriptions.clear();

  const std::vector<Cheat> cheats = CServiceBroker::GetGameServices().CheatRuntime().GetCheats();
  for (size_t index = 0; index < cheats.size(); ++index)
  {
    const Cheat& cheat = cheats[index];
    const std::string id = SettingId(index);

    // A cheat file is allowed to leave a cheat unnamed, and a switch with no
    // label cannot be told apart from the ones around it
    std::string label = !cheat.description.empty() ? cheat.description : cheat.code;
    if (StringUtils::utf8_strlen(label) > MAX_LABEL_LENGTH)
      label = TruncateCharacters(label, MAX_LABEL_LENGTH - 1) + "\u2026";

    m_labels[id] = std::move(label);

    if (!cheat.longDescription.empty())
      m_descriptions[id] = cheat.longDescription;
    else if (StringUtils::utf8_strlen(cheat.description) > MAX_LABEL_LENGTH)
      m_descriptions[id] = cheat.description;

    // GetSettingsLabel() supplies the real label; a setting still has to be
    // given a string ID to be created at all
    AddToggle(group, id, HEADING_CHEATS, SettingLevel::Basic, cheat.enabled);
  }
}

void CDialogGameCheats::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string& id = setting->GetId();
  if (!StringUtils::StartsWith(id, SETTING_CHEAT_PREFIX))
    return;

  const std::string index = id.substr(std::strlen(SETTING_CHEAT_PREFIX));
  if (index.empty() || !StringUtils::IsNaturalNumber(index))
    return;

  CServiceBroker::GetGameServices().CheatRuntime().SetEnabled(
      static_cast<unsigned int>(std::stoul(index)),
      std::static_pointer_cast<const CSettingBool>(setting)->GetValue());
}
