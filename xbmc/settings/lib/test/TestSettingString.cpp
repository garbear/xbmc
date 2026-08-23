/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/Setting.h"
#include "utils/XBMCTinyXML.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
std::shared_ptr<CSettingString> DeserializeStringSetting(const std::string& xml)
{
  CXBMCTinyXML document;
  if (!document.Parse(xml))
    return {};

  auto setting = std::make_shared<CSettingString>("test.string");
  if (!setting->Deserialize(document.RootElement(), true))
    return {};

  return setting;
}

StringSettingOptions GetExposedOptions(const std::shared_ptr<CSettingString>& setting)
{
  StringSettingOptions options;
  switch (setting->GetOptionsType())
  {
    case SettingOptionsType::StaticTranslatable:
      for (const auto& option : setting->GetTranslatableOptions())
      {
        options.emplace_back(option.IsTranslatable() ? "localized " + std::to_string(option.label)
                                                     : option.labelText,
                             option.value);
      }
      break;

    case SettingOptionsType::Static:
      options = setting->GetOptions();
      break;

    default:
      break;
  }

  return options;
}
} // unnamed namespace

TEST(TestSettingString, DeserializeMixedStaticOptions)
{
  const auto setting = DeserializeStringSetting(R"(
    <setting id="test.string" type="string">
      <default>disabled</default>
      <constraints>
        <options>
          <option>disabled</option>
          <option label="30001">top/bottom</option>
          <option label="Literal label">literal</option>
          <option label="30002">full</option>
        </options>
      </constraints>
    </setting>)");
  ASSERT_NE(nullptr, setting);
  EXPECT_EQ(SettingOptionsType::StaticTranslatable, setting->GetOptionsType());

  const TranslatableStringSettingOptions& storedOptions = setting->GetTranslatableOptions();
  ASSERT_EQ(4U, storedOptions.size());
  EXPECT_FALSE(storedOptions[0].IsTranslatable());
  EXPECT_TRUE(storedOptions[1].IsTranslatable());
  EXPECT_FALSE(storedOptions[2].IsTranslatable());
  EXPECT_TRUE(storedOptions[3].IsTranslatable());

  const StringSettingOptions options = GetExposedOptions(setting);
  ASSERT_EQ(4U, options.size());
  EXPECT_EQ("disabled", options[0].label);
  EXPECT_EQ("disabled", options[0].value);
  EXPECT_EQ("localized 30001", options[1].label);
  EXPECT_EQ("top/bottom", options[1].value);
  EXPECT_EQ("Literal label", options[2].label);
  EXPECT_EQ("literal", options[2].value);
  EXPECT_EQ("localized 30002", options[3].label);
  EXPECT_EQ("full", options[3].value);

  EXPECT_TRUE(setting->SetValue("full"));
  EXPECT_TRUE(setting->SetValue("disabled"));
  EXPECT_EQ("disabled", setting->GetValue());

  EXPECT_TRUE(setting->SetValue("full"));
  setting->Reset();
  EXPECT_EQ("disabled", setting->GetValue());
}

TEST(TestSettingString, DeserializeStaticOptions)
{
  const auto setting = DeserializeStringSetting(R"(
    <setting id="test.string" type="string">
      <default>one</default>
      <constraints>
        <options>
          <option>one</option>
          <option label="Second literal">two</option>
        </options>
      </constraints>
    </setting>)");
  ASSERT_NE(nullptr, setting);
  EXPECT_EQ(SettingOptionsType::Static, setting->GetOptionsType());

  const StringSettingOptions options = GetExposedOptions(setting);
  ASSERT_EQ(2U, options.size());
  EXPECT_EQ("one", options[0].label);
  EXPECT_EQ("one", options[0].value);
  EXPECT_EQ("Second literal", options[1].label);
  EXPECT_EQ("two", options[1].value);
}

TEST(TestSettingString, DeserializeTranslatableOptions)
{
  const auto setting = DeserializeStringSetting(R"(
    <setting id="test.string" type="string">
      <default>one</default>
      <constraints>
        <options>
          <option label="30001">one</option>
          <option label="30002">two</option>
        </options>
      </constraints>
    </setting>)");
  ASSERT_NE(nullptr, setting);
  EXPECT_EQ(SettingOptionsType::StaticTranslatable, setting->GetOptionsType());

  const StringSettingOptions options = GetExposedOptions(setting);
  ASSERT_EQ(2U, options.size());
  EXPECT_EQ("localized 30001", options[0].label);
  EXPECT_EQ("one", options[0].value);
  EXPECT_EQ("localized 30002", options[1].label);
  EXPECT_EQ("two", options[1].value);
}
