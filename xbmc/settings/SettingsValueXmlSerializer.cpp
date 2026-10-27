/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsValueXmlSerializer.h"

#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>

static constexpr char SETTINGS_XML_ROOT[] = "settings";

std::string CSettingsValueXmlSerializer::SerializeValues(
  const CSettingsManager* settingsManager) const
{
  if (!settingsManager)
    return "";

  CXBMCTinyXML xmlDoc;
  TiXmlElement rootElement(SETTINGS_XML_ROOT);
  rootElement.SetAttribute(SETTING_XML_ROOT_VERSION, settingsManager->GetVersion());
  TiXmlNode* xmlRoot = xmlDoc.InsertEndChild(rootElement);
  if (!xmlRoot)
    return "";

  const SettingSectionList sections = settingsManager->GetSections();
  for (const auto& section : sections)
    SerializeSection(xmlRoot, section);

  if (const auto* original = settingsManager->GetValues())
  {
    TiXmlDocument merged(*original);
    auto* root = merged.RootElement();
    const auto runningVersion = settingsManager->GetVersion();
    const auto fileVersion = settingsManager->ParseVersion(root);
    root->SetAttribute(SETTING_XML_ROOT_VERSION, std::max(fileVersion, runningVersion));

    for (const auto* value = xmlRoot->FirstChildElement(SETTING_XML_ELM_SETTING); value;
         value = value->NextSiblingElement(SETTING_XML_ELM_SETTING))
    {
      const std::string id = value->Attribute(SETTING_XML_ATTR_ID);
      if (!settingsManager->HasSettingChanged(id))
        continue;

      auto* target = CSettingsManager::LocateSetting(root, id, runningVersion);
      if (fileVersion > runningVersion && (!target || !target->Attribute(SETTING_XML_ROOT_VERSION)))
        target = nullptr;

      if (!target)
      {
        target = root->InsertEndChild(*value)->ToElement();
        if (fileVersion > runningVersion)
          target->SetAttribute(SETTING_XML_ROOT_VERSION, runningVersion);
      }
      else
      {
        if (auto* text = target->FirstChild(); text && text->ToText())
          text->SetValue(value->GetText() ? value->GetText() : "");
        else
          target->InsertEndChild(TiXmlText(value->GetText() ? value->GetText() : ""));
        if (const char* isDefault = value->Attribute(SETTING_XML_ELM_DEFAULT))
          target->SetAttribute(SETTING_XML_ELM_DEFAULT, isDefault);
        else
          target->RemoveAttribute(SETTING_XML_ELM_DEFAULT);
      }
    }

    std::stringstream stream;
    stream << merged;
    return stream.str();
  }

  std::stringstream stream;
  stream << xmlDoc;

  return stream.str();
}

void CSettingsValueXmlSerializer::SerializeSection(
    TiXmlNode* parent, const std::shared_ptr<CSettingSection>& section) const
{
  if (!section)
    return;

  const SettingCategoryList& categories = section->GetCategories();
  for (const auto& category : categories)
    SerializeCategory(parent, category);
}

void CSettingsValueXmlSerializer::SerializeCategory(
    TiXmlNode* parent, const std::shared_ptr<CSettingCategory>& category) const
{
  if (!category)
    return;

  const SettingGroupList& groups = category->GetGroups();
  for (const auto& group : groups)
    SerializeGroup(parent, group);
}

void CSettingsValueXmlSerializer::SerializeGroup(TiXmlNode* parent,
                                                 const std::shared_ptr<CSettingGroup>& group) const
{
  if (!group)
    return;

  const SettingList& settings = group->GetSettings();
  for (const auto& setting : settings)
    SerializeSetting(parent, setting);
}

bool CSettingsValueXmlSerializer::SerializeSetting(TiXmlNode* parent,
                                                   const std::shared_ptr<CSetting>& setting)
{
  if (!setting)
    return true;

  // ignore references and action settings (which don't have a value)
  if (setting->IsReference() || setting->GetType() == SettingType::Action)
    return true;

  TiXmlElement settingElement(SETTING_XML_ELM_SETTING);
  settingElement.SetAttribute(SETTING_XML_ATTR_ID, setting->GetId());

  // add the default attribute
  if (setting->IsDefault())
    settingElement.SetAttribute(SETTING_XML_ELM_DEFAULT, "true");

  // add the value
  TiXmlText value(setting->ToString());

  if (settingElement.InsertEndChild(value) == nullptr ||
      parent->InsertEndChild(settingElement) == nullptr)
  {
    CLog::Log(LOGWARNING, "CSettingsValueXmlSerializer: unable to write <{} id=\"{}\"> tag",
              SETTING_XML_ELM_SETTING, setting->GetId());
    return false;
  }
  return true;
}
