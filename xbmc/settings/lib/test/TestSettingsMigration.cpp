/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/ISettingsMigrationStep.h"
#include "settings/lib/SettingsManager.h"
#include "settings/lib/SettingsMigration.h"
#include "settings/lib/SettingsMigrationPrimitive.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::SETTINGS;

struct StepSpec
{
  int version;
  bool returns{true};
};

struct UpdateXMLSettingsTest
{
  std::vector<StepSpec> steps;
  int currentVersion;
  int targetVersion;
  bool expectedReturn;
  std::vector<int> expectedCompleted;
};

const UpdateXMLSettingsTest UpdateXMLSettingsTests[]{
    // "normal" guisettings.xml cases - contiguous steps from version 2
    {{{3}}, 2, 3, true, {3}},
    {{{3}, {4}}, 2, 4, true, {3, 4}},

    // advancedsettings.xml typically doesn't have a version number => version 0
    {{{3}}, 0, 3, true, {3}},

    // steps below or above the range are skipped
    {{{3}, {4}, {5}}, 3, 4, true, {4}},
    {{{3}, {4}, {5}}, 4, 5, true, {5}},

    // gaps in the steps
    {{{3}, {5}}, 2, 5, true, {3, 5}},

    // steps exist only outside of the range
    {{{1}}, 2, 3, true, {}},
    {{{10}}, 2, 3, true, {}},

    // no steps
    {{}, 2, 3, true, {}},

    // steps provided out of order
    {{{5}, {3}, {4}}, 2, 5, true, {3, 4, 5}},

    // stop on first error
    {{{3, false}, {4}}, 2, 4, false, {3}},
    {{{3}, {4, false}}, 2, 4, false, {3, 4}},

    // no work under the minimum version LEGACY_VERSION = 2 or already on current version
    {{{0}, {1}, {2}}, 0, 2, true, {}},
    {{}, 2, 2, true, {}},
    {{{3}}, 3, 3, true, {}},
};

class TestSettingsMigrationUpdateXML : public testing::WithParamInterface<UpdateXMLSettingsTest>,
                                       public testing::Test
{
};

struct StubStep : ISettingsMigrationStep
{
  StubStep(int version, bool returnValue, std::vector<int>& completionLog)
    : m_version(version),
      m_returnValue(returnValue),
      m_log(completionLog)
  {
  }
  int TargetVersion() const override { return m_version; }
  bool Apply(TiXmlElement*) override
  {
    m_log.push_back(m_version);
    return m_returnValue;
  }

private:
  int m_version;
  bool m_returnValue{true};
  std::vector<int>& m_log;
};

TEST_P(TestSettingsMigrationUpdateXML, UpdateXMLSettings)
{
  const auto& params = GetParam();

  std::vector<int> completed;

  MigrationStepList steps;
  for (const auto& step : params.steps)
    steps.push_back(std::make_shared<StubStep>(step.version, step.returns, completed));

  CSettingsMigration migration(std::move(steps));

  EXPECT_EQ(params.expectedReturn,
            migration.UpdateXMLSettings(nullptr, params.currentVersion, params.targetVersion));
  EXPECT_EQ(params.expectedCompleted, completed);
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestSettingsMigrationUpdateXML,
                         testing::ValuesIn(UpdateXMLSettingsTests));

TEST(TestSettingsMigration, MultipleMigrationStepsSameTarget)
{
  std::vector<int> completed;

  MigrationStepList steps;
  steps.push_back(std::make_shared<StubStep>(3, true, completed));
  steps.push_back(std::make_shared<StubStep>(3, true, completed));

  EXPECT_THROW({ CSettingsMigration(std::move(steps)); }, std::invalid_argument);
}

namespace
{
struct VideoVersionsMigrationTest
{
  std::string_view settings;
  std::string_view expectedSettings;
};

constexpr VideoVersionsMigrationTest VideoVersionsMigrationTests[]{
    {R"(<setting id="videolibrary.ignorevideoversions">true</setting>)",
     R"(<setting id="videolibrary.ignorevideoversions" version="2">true</setting>)"
     R"(<setting id="videolibrary.similarvideoaction">0</setting>)"},
    {R"(<setting id="videolibrary.ignorevideoversions">false</setting>)",
     R"(<setting id="videolibrary.ignorevideoversions" version="2">false</setting>)"
     R"(<setting id="videolibrary.similarvideoaction">1</setting>)"},
    {R"(<setting id="videolibrary.ignorevideoversions" default="true">true</setting>)",
     R"(<setting id="videolibrary.ignorevideoversions" default="true" version="2">true</setting>)"
     R"(<setting id="videolibrary.similarvideoaction">0</setting>)"},
    {"", ""},
    {R"(<setting id="videolibrary.ignorevideoversions">true</setting>)"
     R"(<setting id="videolibrary.similarvideoaction">1</setting>)",
     R"(<setting id="videolibrary.ignorevideoversions" version="2">true</setting>)"
     R"(<setting id="videolibrary.similarvideoaction">1</setting>)"},
    {R"(<setting id="videolibrary.ignorevideoversions">false</setting>)"
     R"(<setting id="videolibrary.similarvideoaction" default="true">2</setting>)"
     R"(<setting id="dvds.autorun">true</setting>)",
     R"(<setting id="videolibrary.ignorevideoversions" version="2">false</setting>)"
     R"(<setting id="videolibrary.similarvideoaction" default="true">2</setting>)"
     R"(<setting id="dvds.autorun" version="2">true</setting>)"
     R"(<setting id="dvds.autoaction">1</setting>)"},
};
} // namespace

class TestSettingsMigrationToV3VideoVersions
  : public testing::WithParamInterface<VideoVersionsMigrationTest>,
    public testing::Test
{
};

TEST_P(TestSettingsMigrationToV3VideoVersions, MigratesVideoVersionPreference)
{
  const auto& params = GetParam();
  const std::string prefix = R"(<settings version="2">)"
                             R"(<setting id="lookandfeel.skin">skin.estuary</setting>)";
  const std::string xml = prefix + std::string{params.settings} + "</settings>";
  const std::string expectedXml = prefix + std::string{params.expectedSettings} + "</settings>";

  for (const int targetVersion : {3, static_cast<int>(CSettingsManager::Version)})
  {
    SCOPED_TRACE(targetVersion);
    CXBMCTinyXML doc;
    ASSERT_TRUE(doc.Parse(xml));

    CSettingsMigration migration;
    ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, targetVersion));

    EXPECT_EQ(expectedXml, XMLUtils::NodeStringSerialization(
                               doc.RootElement(), XMLUtils::SerializationFormat::COMPACT));
  }
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestSettingsMigrationToV3VideoVersions,
                         testing::ValuesIn(VideoVersionsMigrationTests));

namespace
{
struct ReplayGainMigrationTest
{
  std::string_view oldSettingId;
  std::string_view newSettingId;
  int oldValue;
  double expectedValue;
};

constexpr ReplayGainMigrationTest ReplayGainMigrationTests[]{
    {"musicplayer.replaygainpreamp", "musicplayer.replaygainpreampdb", 77, -12.0},
    {"musicplayer.replaygainpreamp", "musicplayer.replaygainpreampdb", 89, 0.0},
    {"musicplayer.replaygainpreamp", "musicplayer.replaygainpreampdb", 101, 12.0},
    {"musicplayer.replaygainnogainpreamp", "musicplayer.replaygainnogainpreampdb", 77, -12.0},
    {"musicplayer.replaygainnogainpreamp", "musicplayer.replaygainnogainpreampdb", 89, 0.0},
    {"musicplayer.replaygainnogainpreamp", "musicplayer.replaygainnogainpreampdb", 101, 12.0},
};
} // namespace

class TestSettingsMigrationToV4ReplayGain
  : public testing::WithParamInterface<ReplayGainMigrationTest>,
    public testing::Test
{
};

TEST_P(TestSettingsMigrationToV4ReplayGain, MigratesPreampSetting)
{
  const auto& params = GetParam();
  const std::string xml =
      "<settings version=\"3\"><setting id=\"" + std::string{params.oldSettingId} + "\">" +
      std::to_string(params.oldValue) +
      "</setting><setting id=\"lookandfeel.skin\">skin.estuary</setting></settings>";

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(xml));

  CSettingsMigration migration;
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 3, 4));

  const auto* legacy = CSettingsManager::LocateSetting(doc.RootElement(), params.oldSettingId, 3);
  ASSERT_NE(nullptr, legacy);
  EXPECT_STREQ("3", legacy->Attribute("version"));
  EXPECT_EQ(std::to_string(params.oldValue), legacy->GetText());

  const TiXmlElement* migratedSetting =
      CSettingsManager::LocateSetting(doc.RootElement(), params.newSettingId);
  ASSERT_NE(nullptr, migratedSetting);
  ASSERT_NE(nullptr, migratedSetting->FirstChild());
  EXPECT_DOUBLE_EQ(params.expectedValue, std::stod(migratedSetting->FirstChild()->ValueStr()));

  const TiXmlElement* unrelatedSetting =
      CSettingsManager::LocateSetting(doc.RootElement(), "lookandfeel.skin");
  ASSERT_NE(nullptr, unrelatedSetting);
  ASSERT_NE(nullptr, unrelatedSetting->FirstChild());
  EXPECT_EQ("skin.estuary", unrelatedSetting->FirstChild()->ValueStr());
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestSettingsMigrationToV4ReplayGain,
                         testing::ValuesIn(ReplayGainMigrationTests));

class TestSettingsMigrationToV5ApplicationKeyboard : public testing::TestWithParam<std::string>
{
};

TEST_P(TestSettingsMigrationToV5ApplicationKeyboard, MigratesTvosKeyboardSetting)
{
  const std::string& value = GetParam();
  const std::string xml = "<settings version=\"4\"><setting id=\"input.tvosusekodikeyboard\">" +
                          value + "</setting></settings>";

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(xml));

  CSettingsMigration migration;
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 4, 5));

  const auto* legacy =
      CSettingsManager::LocateSetting(doc.RootElement(), "input.tvosusekodikeyboard", 4);
  ASSERT_NE(nullptr, legacy);
  EXPECT_STREQ("4", legacy->Attribute("version"));
  EXPECT_EQ(value, legacy->GetText());

  const TiXmlElement* migratedSetting =
      CSettingsManager::LocateSetting(doc.RootElement(), "input.useapplicationkeyboard");
  ASSERT_NE(nullptr, migratedSetting);
  ASSERT_NE(nullptr, migratedSetting->FirstChild());
  EXPECT_EQ(value, migratedSetting->FirstChild()->ValueStr());
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestSettingsMigrationToV5ApplicationKeyboard,
                         testing::Values("true", "false"));

TEST(TestSettingsMigration, FullUpgradePreservesEverySource)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{
      R"(<settings version="2"><setting id="dvds.autorun">true</setting>
         <setting id="videolibrary.ignorevideoversions">false</setting>
         <setting id="musicplayer.replaygainpreamp">77</setting>
         <setting id="musicplayer.replaygainnogainpreamp">101</setting>
         <setting id="input.tvosusekodikeyboard">true</setting>
         <setting id="lookandfeel.skin">skin.estuary</setting></settings>)"}));
  CSettingsMigration migration;
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, 5));
  struct ExpectedSetting
  {
    const char* id;
    const char* value;
    unsigned int version;
  };
  const ExpectedSetting sources[]{
      {"dvds.autorun", "true", 2},
      {"videolibrary.ignorevideoversions", "false", 2},
      {"musicplayer.replaygainpreamp", "77", 3},
      {"musicplayer.replaygainnogainpreamp", "101", 3},
      {"input.tvosusekodikeyboard", "true", 4},
  };
  for (const auto& source : sources)
  {
    const auto* setting =
        CSettingsManager::LocateSetting(doc.RootElement(), source.id, source.version);
    ASSERT_NE(nullptr, setting) << source.id;
    EXPECT_STREQ(source.value, setting->GetText());
    unsigned int version = 0;
    EXPECT_EQ(TIXML_SUCCESS, setting->QueryUnsignedAttribute("version", &version));
    EXPECT_EQ(source.version, version);
  }
  for (const char* id :
       {"dvds.autoaction", "videolibrary.similarvideoaction", "musicplayer.replaygainpreampdb",
        "musicplayer.replaygainnogainpreampdb", "input.useapplicationkeyboard", "lookandfeel.skin"})
  {
    const auto* setting = CSettingsManager::LocateSetting(doc.RootElement(), id);
    ASSERT_NE(nullptr, setting) << id;
    EXPECT_EQ(nullptr, setting->Attribute("version"));
  }
  const auto expected =
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT);
  for (int i = 0; i < 3; ++i)
  {
    ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, 5));
    EXPECT_EQ(expected, XMLUtils::NodeStringSerialization(doc.RootElement(),
                                                          XMLUtils::SerializationFormat::COMPACT));
  }
}

TEST(TestSettingsMigration, ExistingDestinationSurvivesLegacyEdits)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{
      R"(<settings version="4"><setting id="input.tvosusekodikeyboard">false</setting>
         <setting id="input.useapplicationkeyboard" extra="keep">true</setting></settings>)"}));
  CSettingsMigration migration;
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 4, 5));
  const auto* current =
      CSettingsManager::LocateSetting(doc.RootElement(), "input.useapplicationkeyboard");
  ASSERT_NE(nullptr, current);
  EXPECT_STREQ("true", current->GetText());
  EXPECT_STREQ("keep", current->Attribute("extra"));
  auto* legacy = CSettingsManager::LocateSetting(doc.RootElement(), "input.tvosusekodikeyboard", 4);
  ASSERT_NE(nullptr, legacy);
  EXPECT_STREQ("4", legacy->Attribute("version"));
  legacy->FirstChild()->SetValue("true");
  current = CSettingsManager::LocateSetting(doc.RootElement(), "input.useapplicationkeyboard");
  const auto expected =
      XMLUtils::NodeStringSerialization(current, XMLUtils::SerializationFormat::COMPACT);
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 4, 5));
  EXPECT_EQ(expected,
            XMLUtils::NodeStringSerialization(
                CSettingsManager::LocateSetting(doc.RootElement(), "input.useapplicationkeyboard"),
                XMLUtils::SerializationFormat::COMPACT));
}

namespace
{
class CSameIdToV3 : public ISettingsMigrationStep
{
public:
  int TargetVersion() const override { return 3; }
  bool Apply(TiXmlElement* root) override
  {
    return ConvertSettingBoolToInt(root, 2, "test.value", "test.value",
                                   {.m_default = 0, .m_false = 0, .m_true = 1}) !=
           SettingConversionResult::FAILURE;
  }
};

class CSameIdToV4 : public ISettingsMigrationStep
{
public:
  int TargetVersion() const override { return 4; }
  bool Apply(TiXmlElement* root) override
  {
    return impl::ConvertSingleSetting<CSettingInt, CSettingNumber>(
               root, 3, "test.value", "test.value", [](int value)
               { return std::pair{value + 0.5, 0.0}; }) != SettingConversionResult::FAILURE;
  }
};
} // namespace

TEST(TestSettingsMigration, ConsecutiveSameIdConversionsPreserveIntermediateValues)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{
      R"(<settings version="2"><setting id="test.value" extra="keep">true</setting></settings>)"}));
  CSettingsMigration migration({std::make_shared<CSameIdToV3>(), std::make_shared<CSameIdToV4>()});
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, 4));
  doc.RootElement()->SetAttribute("version", 4);
  const auto* v2 = CSettingsManager::LocateSetting(doc.RootElement(), "test.value", 2);
  const auto* v3 = CSettingsManager::LocateSetting(doc.RootElement(), "test.value", 3);
  const auto* v4 = CSettingsManager::LocateSetting(doc.RootElement(), "test.value", 4);
  ASSERT_NE(nullptr, v2);
  ASSERT_NE(nullptr, v3);
  ASSERT_NE(nullptr, v4);
  EXPECT_STREQ("true", v2->GetText());
  EXPECT_STREQ("keep", v2->Attribute("extra"));
  EXPECT_STREQ("2", v2->Attribute("version"));
  EXPECT_STREQ("1", v3->GetText());
  EXPECT_STREQ("3", v3->Attribute("version"));
  EXPECT_DOUBLE_EQ(1.5, std::stod(v4->GetText()));
  EXPECT_EQ(nullptr, v4->Attribute("version"));
  const auto expected =
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT);
  for (int i = 0; i < 3; ++i)
  {
    ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, 4));
    EXPECT_EQ(expected, XMLUtils::NodeStringSerialization(doc.RootElement(),
                                                          XMLUtils::SerializationFormat::COMPACT));
  }
}

TEST(TestSettingsMigration, ExistingIntermediateDestinationIsNotReplaced)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{
      R"(<settings version="4"><setting id="test.value" version="2">true</setting>
         <setting id="test.value" version="3">7</setting></settings>)"}));
  CSettingsMigration migration({std::make_shared<CSameIdToV3>(), std::make_shared<CSameIdToV4>()});
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, 4));
  const auto* v3 = CSettingsManager::LocateSetting(doc.RootElement(), "test.value", 3);
  const auto* v4 = CSettingsManager::LocateSetting(doc.RootElement(), "test.value", 4);
  ASSERT_NE(nullptr, v3);
  ASSERT_NE(nullptr, v4);
  EXPECT_STREQ("7", v3->GetText());
  EXPECT_DOUBLE_EQ(7.5, std::stod(v4->GetText()));
}

TEST(TestSettingsMigration, MissingAndInvalidSourcesDoNotDamageOtherSettings)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{
      R"(<settings version="2"><setting id="dvds.autorun">invalid</setting>
         <setting id="videolibrary.ignorevideoversions">invalid</setting>
         <setting id="musicplayer.replaygainpreamp">invalid</setting>
         <setting id="input.tvosusekodikeyboard">invalid</setting>
         <future data="keep" /></settings>)"}));
  const auto expected =
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT);
  CSettingsMigration migration;
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, 5));
  EXPECT_EQ(expected, XMLUtils::NodeStringSerialization(doc.RootElement(),
                                                        XMLUtils::SerializationFormat::COMPACT));
}

TEST(TestSettingsMigration, MissingCurrentValueCompletesConsecutiveMigrations)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{
      R"(<settings version="4"><setting id="test.value" version="2">true</setting></settings>)"}));
  CSettingsMigration migration({std::make_shared<CSameIdToV3>(), std::make_shared<CSameIdToV4>()});
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, 4));
  const auto* intermediate = CSettingsManager::LocateSetting(doc.RootElement(), "test.value", 3);
  const auto* current = CSettingsManager::LocateSetting(doc.RootElement(), "test.value", 4);
  ASSERT_NE(nullptr, intermediate);
  ASSERT_NE(nullptr, current);
  EXPECT_STREQ("3", intermediate->Attribute("version"));
  EXPECT_STREQ("1", intermediate->GetText());
  EXPECT_DOUBLE_EQ(1.5, std::stod(current->GetText()));
  EXPECT_EQ(nullptr, current->Attribute("version"));
  EXPECT_STREQ("4", doc.RootElement()->Attribute("version"));
}

TEST(TestSettingsMigration, ExistingCurrentValueIsNotTreatedAsAnIntermediate)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{
      R"(<settings version="4"><setting id="test.value" version="2">true</setting>
         <setting id="test.value">9.5</setting></settings>)"}));
  const auto expected =
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT);
  CSettingsMigration migration({std::make_shared<CSameIdToV3>(), std::make_shared<CSameIdToV4>()});
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, 4));
  EXPECT_EQ(expected, XMLUtils::NodeStringSerialization(doc.RootElement(),
                                                        XMLUtils::SerializationFormat::COMPACT));
}
