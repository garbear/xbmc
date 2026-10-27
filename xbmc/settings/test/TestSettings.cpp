/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "application/ApplicationSettingsHandling.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "view/ViewStateSettings.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#if defined(TARGET_DARWIN_OSX)
#include <sys/stat.h>
#endif

class TestSettingsPersistence : public testing::Test
{
protected:
  void SetUp() override
  {
    m_file = CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetSettingsFile();
    ASSERT_FALSE(XFILE::CFile::Exists(m_file));
    m_setting = std::make_shared<CSettingBool>("test.value", nullptr);
    ASSERT_TRUE(m_settings.GetSettingsManager()->AddSetting(
        m_setting, std::make_shared<CSettingSection>("test"),
        std::make_shared<CSettingCategory>("test"), std::make_shared<CSettingGroup>("test")));
    m_settings.GetSettingsManager()->SetInitialized();
  }

  void TearDown() override { XFILE::CFile::Delete(m_file); }

  void Write(const std::string& xml)
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(m_file, true));
    ASSERT_EQ(static_cast<ssize_t>(xml.size()), file.Write(xml.data(), xml.size()));
  }

  std::string Read() const
  {
    XFILE::CFile file;
    if (!file.Open(m_file))
      return {};
    std::string xml(file.GetLength(), '\0');
    file.Read(xml.data(), xml.size());
    return xml;
  }

  std::set<std::string> TemporaryFiles() const
  {
    const auto path = std::filesystem::u8path(CSpecialProtocol::TranslatePath(m_file));
    std::set<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(path.parent_path()))
    {
      const auto name = entry.path().filename().string();
      if (name.find(path.filename().string() + ".") == 0)
        files.insert(name);
    }
    return files;
  }

  CSettings m_settings;
  std::shared_ptr<CSettingBool> m_setting;
  std::string m_file;
};

TEST_F(TestSettingsPersistence, FutureVersionDoesNotResetFile)
{
  const std::string xml = "<settings version=\"" + std::to_string(CSettingsManager::Version + 1) +
                          "\"><setting id=\"future.value\" future=\"yes\">keep</setting>"
                          "<future-section /></settings>";
  Write(xml);
  EXPECT_TRUE(m_settings.Load(m_file));
  EXPECT_EQ(xml, Read());
}

TEST_F(TestSettingsPersistence, LoadsCurrentVersion)
{
  const std::string xml = "<settings version=\"" + std::to_string(CSettingsManager::Version) +
                          "\"><setting id=\"test.value\">true</setting></settings>";
  Write(xml);
  EXPECT_TRUE(m_settings.Load(m_file));
  EXPECT_TRUE(m_setting->GetValue());
  EXPECT_EQ(xml, Read());
}

TEST_F(TestSettingsPersistence, RecoversMissingFile)
{
  EXPECT_TRUE(m_settings.Load(m_file));
  EXPECT_FALSE(m_setting->GetValue());
  EXPECT_FALSE(Read().empty());
}

TEST_F(TestSettingsPersistence, RecoversMalformedFile)
{
  Write("<settings><broken");
  EXPECT_TRUE(m_settings.Load(m_file));
  EXPECT_FALSE(m_setting->GetValue());
  EXPECT_NE("<settings><broken", Read());
}

namespace
{
class CTestSubSettings : public ISubSettings
{
public:
  bool Load(const TiXmlNode*) override { return true; }
  bool Save(TiXmlNode* root) const override
  {
    TiXmlElement section("defaultvideosettings");
    auto* node = root->InsertEndChild(section);
    XMLUtils::SetInt(node, "viewmode", m_value);
    return node != nullptr;
  }
  void Clear() override {}
  int m_value{0};
};
} // namespace

TEST_F(TestSettingsPersistence, SubSettingsPreserveUnknownXmlWithoutDuplicateSections)
{
  CTestSubSettings subSettings;
  m_settings.RegisterSubSettings(&subSettings);
  Write("<?xml version=\"1.0\"?><!--keep--><settings version=\"" +
        std::to_string(CSettingsManager::Version) +
        "\"><defaultvideosettings future=\"yes\"><viewmode extra=\"keep\">0</viewmode>"
        "<future>keep</future></defaultvideosettings><unknown /></settings><!--after-->");
  ASSERT_TRUE(m_settings.Load(m_file));
  subSettings.m_value = 2;
  for (int i = 0; i < 3; ++i)
  {
    ASSERT_TRUE(m_settings.Save(m_file));
    CXBMCTinyXML xml;
    ASSERT_TRUE(xml.LoadFile(m_file));
    const auto* section = xml.RootElement()->FirstChildElement("defaultvideosettings");
    ASSERT_NE(nullptr, section);
    EXPECT_STREQ("yes", section->Attribute("future"));
    EXPECT_STREQ("keep", section->FirstChildElement("viewmode")->Attribute("extra"));
    EXPECT_STREQ("2", section->FirstChildElement("viewmode")->GetText());
    EXPECT_NE(nullptr, section->FirstChildElement("future"));
    EXPECT_EQ(nullptr, section->NextSiblingElement("defaultvideosettings"));
    EXPECT_NE(nullptr, xml.RootElement()->FirstChildElement("unknown"));
    EXPECT_NE(std::string::npos, Read().find("<!--keep-->"));
    EXPECT_NE(std::string::npos, Read().find("<!--after-->"));
  }
  m_settings.UnregisterSubSettings(&subSettings);
}

TEST_F(TestSettingsPersistence, StaleTemporaryFileDoesNotBlockSave)
{
  const std::string stale = m_file + ".tmp";
  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(stale, true));
  ASSERT_EQ(5, file.Write("stale", 5));
  file.Close();
  const auto temporaryFiles = TemporaryFiles();
  EXPECT_TRUE(m_settings.Save(m_file));
  EXPECT_EQ(temporaryFiles, TemporaryFiles());
  ASSERT_TRUE(file.Open(stale));
  char data[5];
  ASSERT_EQ(5, file.Read(data, sizeof(data)));
  EXPECT_EQ("stale", std::string(data, sizeof(data)));
  file.Close();
  EXPECT_TRUE(XFILE::CFile::Delete(stale));
}

TEST_F(TestSettingsPersistence, FailedReplacementCleansTemporaryFileAndRecovers)
{
  const auto target = std::filesystem::u8path(CSpecialProtocol::TranslatePath(m_file));
  ASSERT_TRUE(std::filesystem::create_directory(target));
  const auto temporaryFiles = TemporaryFiles();
  EXPECT_FALSE(m_settings.Save(m_file));
  EXPECT_TRUE(std::filesystem::is_directory(target));
  EXPECT_EQ(temporaryFiles, TemporaryFiles());
  ASSERT_TRUE(std::filesystem::remove(target));
  EXPECT_TRUE(m_settings.Save(m_file));
  CXBMCTinyXML saved;
  EXPECT_TRUE(saved.LoadFile(m_file));
}

#if defined(TARGET_POSIX)
TEST_F(TestSettingsPersistence, FailedWritePreservesOriginalFileAndRecovers)
{
  const std::string xml = "<settings version=\"" + std::to_string(CSettingsManager::Version) +
                          "\"><setting id=\"test.value\">false</setting></settings>";
  Write(xml);
  ASSERT_TRUE(m_settings.Load(m_file));
  ASSERT_TRUE(m_setting->SetValue(true));
  const auto parent =
      std::filesystem::u8path(CSpecialProtocol::TranslatePath(m_file)).parent_path();
  const auto permissions = std::filesystem::status(parent).permissions();
  const auto temporaryFiles = TemporaryFiles();
  std::filesystem::permissions(parent, std::filesystem::perms::owner_read |
                                           std::filesystem::perms::owner_exec);
  const bool saved = m_settings.Save(m_file);
  std::filesystem::permissions(parent, permissions);
  EXPECT_FALSE(saved);
  EXPECT_EQ(xml, Read());
  EXPECT_EQ(temporaryFiles, TemporaryFiles());
  EXPECT_TRUE(m_settings.Save(m_file));
}
#endif

namespace
{
class CConcurrentSubSettings : public CTestSubSettings
{
public:
  bool Save(TiXmlNode* root) const override
  {
    if (m_active.fetch_add(1) != 0)
      m_overlapped = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const bool result = CTestSubSettings::Save(root);
    --m_active;
    return result;
  }
  mutable std::atomic<int> m_active{0};
  mutable std::atomic<bool> m_overlapped{false};
};

class CFailingSubSettings : public CTestSubSettings
{
public:
  bool Save(TiXmlNode*) const override { return false; }
};
} // namespace

TEST_F(TestSettingsPersistence, FailedSerializationPreservesOriginalFile)
{
  const std::string xml = "<settings version=\"" + std::to_string(CSettingsManager::Version) +
                          "\"><setting id=\"test.value\">false</setting></settings>";
  Write(xml);
  ASSERT_TRUE(m_settings.Load(m_file));
  const auto temporaryFiles = TemporaryFiles();
  CFailingSubSettings subSettings;
  m_settings.RegisterSubSettings(&subSettings);
  EXPECT_FALSE(m_settings.Save(m_file));
  EXPECT_EQ(xml, Read());
  EXPECT_EQ(temporaryFiles, TemporaryFiles());
  m_settings.UnregisterSubSettings(&subSettings);
  EXPECT_TRUE(m_settings.Save(m_file));
}

TEST_F(TestSettingsPersistence, ConcurrentSavesAreSerializedAcrossSettingsInstances)
{
  CSettings otherSettings;
  auto value = std::make_shared<CSettingBool>("test.value", nullptr);
  value->SetValue(true);
  ASSERT_TRUE(otherSettings.GetSettingsManager()->AddSetting(
      value, std::make_shared<CSettingSection>("test"), std::make_shared<CSettingCategory>("test"),
      std::make_shared<CSettingGroup>("test")));
  otherSettings.GetSettingsManager()->SetInitialized();
  CConcurrentSubSettings subSettings;
  m_settings.RegisterSubSettings(&subSettings);
  otherSettings.RegisterSubSettings(&subSettings);
  const auto temporaryFiles = TemporaryFiles();
  std::atomic<int> failures{0};
  std::vector<std::thread> writers;
  for (int i = 0; i < 8; ++i)
  {
    const CSettings& settings = i % 2 ? m_settings : otherSettings;
    writers.emplace_back(
        [&settings, &failures, this]
        {
          for (int j = 0; j < 5; ++j)
          {
            if (!settings.Save(m_file))
              ++failures;
          }
        });
  }
  for (auto& writer : writers)
    writer.join();
  EXPECT_EQ(0, failures.load());
  EXPECT_FALSE(subSettings.m_overlapped);
  EXPECT_EQ(temporaryFiles, TemporaryFiles());
  CXBMCTinyXML saved;
  ASSERT_TRUE(saved.LoadFile(m_file));
  const auto* setting = saved.RootElement()->FirstChildElement("setting");
  ASSERT_NE(nullptr, setting);
  EXPECT_STREQ("test.value", setting->Attribute("id"));
  EXPECT_TRUE(std::string(setting->GetText()) == "true" ||
              std::string(setting->GetText()) == "false");
  EXPECT_EQ(nullptr, setting->NextSiblingElement("setting"));
  m_settings.UnregisterSubSettings(&subSettings);
  otherSettings.UnregisterSubSettings(&subSettings);
}

#if defined(TARGET_DARWIN_OSX)
TEST_F(TestSettingsPersistence, FailedReplacementPreservesOriginalFile)
{
  const std::string xml = "<settings version=\"" + std::to_string(CSettingsManager::Version) +
                          "\"><setting id=\"test.value\">false</setting></settings>";
  Write(xml);
  ASSERT_TRUE(m_settings.Load(m_file));
  ASSERT_TRUE(m_setting->SetValue(true));
  const auto target = CSpecialProtocol::TranslatePath(m_file);
  const auto temporaryFiles = TemporaryFiles();
  ASSERT_EQ(0, chflags(target.c_str(), UF_IMMUTABLE));
  const bool saved = m_settings.Save(m_file);
  ASSERT_EQ(0, chflags(target.c_str(), 0));
  EXPECT_FALSE(saved);
  EXPECT_EQ(xml, Read());
  EXPECT_EQ(temporaryFiles, TemporaryFiles());
  EXPECT_TRUE(m_settings.Save(m_file));
}

TEST_F(TestSettingsPersistence, MacAudioDeviceConversionSurvivesReload)
{
  auto audio =
      std::make_shared<CSettingString>(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE, nullptr);
  CXBMCTinyXML definition;
  ASSERT_TRUE(definition.Parse(std::string{
      R"(<setting id="audiooutput.audiodevice"><updates><update type="change" /></updates></setting>)"}));
  ASSERT_TRUE(audio->Deserialize(definition.RootElement(), true));
  ASSERT_TRUE(m_settings.GetSettingsManager()->AddSetting(
      audio, std::make_shared<CSettingSection>("audio"),
      std::make_shared<CSettingCategory>("audio"), std::make_shared<CSettingGroup>("audio")));
  CApplicationSettingsHandling callback;
  m_settings.GetSettingsManager()->RegisterCallback(&callback, {audio->GetId()});
  Write("<settings version=\"" + std::to_string(CSettingsManager::Version) +
        "\"><setting id=\"audiooutput.audiodevice\" extra=\"keep\">DARWINOSX:device</setting>"
        "<setting id=\"audiooutput.audiodevice\" version=\"1\">legacy</setting>"
        "<setting id=\"future.unknown\" extra=\"keep\">unknown</setting></settings>");
  ASSERT_TRUE(m_settings.Load(m_file));
  EXPECT_EQ("DARWINOSX:device:stream0", audio->GetValue());
  CXBMCTinyXML saved;
  ASSERT_TRUE(saved.LoadFile(m_file));
  const auto* converted = saved.RootElement()->FirstChildElement("setting");
  EXPECT_STREQ("DARWINOSX:device:stream0", converted->GetText());
  EXPECT_STREQ("keep", converted->Attribute("extra"));
  EXPECT_STREQ("legacy", converted->NextSiblingElement("setting")->GetText());
  EXPECT_STREQ("unknown",
               converted->NextSiblingElement("setting")->NextSiblingElement("setting")->GetText());
  const auto bytes = Read();
  for (int i = 0; i < 3; ++i)
  {
    ASSERT_TRUE(m_settings.Load(m_file));
    EXPECT_EQ("DARWINOSX:device:stream0", audio->GetValue());
    EXPECT_EQ(bytes, Read());
  }
  m_settings.GetSettingsManager()->UnregisterCallback(&callback);
}
#endif

TEST_F(TestSettingsPersistence, RealSubSettingsDoNotAccumulateSections)
{
  auto& display = CDisplaySettings::GetInstance();
  auto& view = CViewStateSettings::GetInstance();
  m_settings.RegisterSubSettings(&display);
  m_settings.RegisterSubSettings(&view);
  Write("<settings version=\"" + std::to_string(CSettingsManager::Version) +
        "\"><resolutions future=\"keep\"><resolution extra=\"keep\">"
        "<description>test mode</description><subtitles>10</subtitles><pixelratio>1</pixelratio>"
        "<future>keep</future></resolution></resolutions>"
        "<viewstates future=\"keep\"><future /></viewstates></settings>");
  ASSERT_TRUE(m_settings.Load(m_file));
  for (int i = 0; i < 3; ++i)
  {
    ASSERT_TRUE(m_settings.Save(m_file));
    CXBMCTinyXML xml;
    ASSERT_TRUE(xml.LoadFile(m_file));
    for (const char* name : {"resolutions", "viewstates"})
    {
      const auto* section = xml.RootElement()->FirstChildElement(name);
      ASSERT_NE(nullptr, section);
      EXPECT_STREQ("keep", section->Attribute("future"));
      EXPECT_EQ(nullptr, section->NextSiblingElement(name));
    }
    EXPECT_TRUE(m_settings.Load(m_file));
  }
  display.ClearCalibrations();
  ASSERT_TRUE(m_settings.Save(m_file));
  CXBMCTinyXML xml;
  ASSERT_TRUE(xml.LoadFile(m_file));
  const auto* resolution =
      xml.RootElement()->FirstChildElement("resolutions")->FirstChildElement("resolution");
  ASSERT_NE(nullptr, resolution);
  EXPECT_STREQ("keep", resolution->Attribute("extra"));
  EXPECT_NE(nullptr, resolution->FirstChildElement("future"));
  EXPECT_EQ(nullptr, resolution->FirstChildElement("subtitles"));
  EXPECT_EQ(nullptr, resolution->FirstChildElement("description"));
  m_settings.UnregisterSubSettings(&display);
  m_settings.UnregisterSubSettings(&view);
}

TEST_F(TestSettingsPersistence, FutureSubSettingsFailureDoesNotResetFile)
{
  auto& display = CDisplaySettings::GetInstance();
  m_settings.RegisterSubSettings(&display);
  const std::string xml = "<settings version=\"" + std::to_string(CSettingsManager::Version + 1) +
                          "\"><future /></settings>";
  Write(xml);
  EXPECT_FALSE(m_settings.Load(m_file));
  EXPECT_EQ(xml, Read());
  m_settings.UnregisterSubSettings(&display);
}

TEST_F(TestSettingsPersistence, SavesThroughPathSubstitution)
{
  const std::string redirected =
      CSpecialProtocol::TranslatePath("special://temp/redirected-settings.xml");
  auto& substitutions =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pathSubstitutions;
  substitutions.emplace_back(CSpecialProtocol::TranslatePath(m_file), redirected);
  EXPECT_TRUE(m_settings.Save(m_file));
  substitutions.pop_back();
  EXPECT_TRUE(XFILE::CFile::Exists(redirected));
  EXPECT_FALSE(XFILE::CFile::Exists(m_file));
  XFILE::CFile::Delete(redirected);
}

TEST_F(TestSettingsPersistence, MigratesMissingCurrentRepresentationInCurrentFile)
{
  const auto action = std::make_shared<CSettingInt>("dvds.autoaction", nullptr);
  ASSERT_TRUE(m_settings.GetSettingsManager()->AddSetting(
      action, std::make_shared<CSettingSection>("dvds"), std::make_shared<CSettingCategory>("dvds"),
      std::make_shared<CSettingGroup>("dvds")));
  Write("<settings version=\"" + std::to_string(CSettingsManager::Version) +
        "\"><setting id=\"dvds.autorun\" version=\"2\">true</setting></settings>");
  ASSERT_TRUE(m_settings.Load(m_file));
  EXPECT_EQ(1, action->GetValue());
  CXBMCTinyXML xml;
  ASSERT_TRUE(xml.LoadFile(m_file));
  const auto* legacy = CSettingsManager::LocateSetting(xml.RootElement(), "dvds.autorun", 2);
  const auto* current = CSettingsManager::LocateSetting(xml.RootElement(), "dvds.autoaction");
  ASSERT_NE(nullptr, legacy);
  ASSERT_NE(nullptr, current);
  EXPECT_STREQ("true", legacy->GetText());
  EXPECT_STREQ("2", legacy->Attribute("version"));
  EXPECT_STREQ("1", current->GetText());
  EXPECT_EQ(nullptr, current->Attribute("version"));
  const auto saved = Read();
  EXPECT_TRUE(m_settings.Load(m_file));
  EXPECT_EQ(saved, Read());
}

TEST_F(TestSettingsPersistence, UpgradeSaveRetainsLegacyAndExistingDestination)
{
  const auto action = std::make_shared<CSettingInt>("dvds.autoaction", nullptr);
  ASSERT_TRUE(m_settings.GetSettingsManager()->AddSetting(
      action, std::make_shared<CSettingSection>("dvds"), std::make_shared<CSettingCategory>("dvds"),
      std::make_shared<CSettingGroup>("dvds")));
  Write(R"(<settings version="2"><setting id="dvds.autorun">true</setting>
           <setting id="dvds.autoaction">0</setting><unknown data="keep" /></settings>)");
  ASSERT_TRUE(m_settings.Load(m_file));
  EXPECT_EQ(0, action->GetValue());
  for (int i = 0; i < 3; ++i)
  {
    CXBMCTinyXML xml;
    ASSERT_TRUE(xml.LoadFile(m_file));
    auto* legacy = CSettingsManager::LocateSetting(xml.RootElement(), "dvds.autorun", 2);
    ASSERT_NE(nullptr, legacy);
    EXPECT_STREQ("2", legacy->Attribute("version"));
    legacy->FirstChild()->SetValue(i % 2 ? "true" : "false");
    ASSERT_TRUE(xml.SaveFile(m_file));
    ASSERT_TRUE(m_settings.Load(m_file));
    EXPECT_EQ(0, action->GetValue());
    ASSERT_TRUE(m_settings.Save(m_file));
    CXBMCTinyXML saved;
    ASSERT_TRUE(saved.LoadFile(m_file));
    EXPECT_EQ(CSettingsManager::Version,
              m_settings.GetSettingsManager()->ParseVersion(saved.RootElement()));
    EXPECT_NE(nullptr, saved.RootElement()->FirstChildElement("unknown"));
    int nodes = 0;
    for (const auto* node = saved.RootElement()->FirstChildElement("setting"); node;
         node = node->NextSiblingElement("setting"))
      ++nodes;
    EXPECT_EQ(2, nodes);
  }
}

namespace
{
class CTestSkin : public ADDON::CSkinInfo
{
public:
  using CSkinInfo::CSkinInfo;
  using CSkinInfo::SettingsFromXML;
  bool SaveSettings(ADDON::AddonInstanceId = ADDON::ADDON_SETTINGS_ID) override { return true; }
};
} // namespace

TEST_F(TestSettingsPersistence, RetainedSkinSettingsDoNotOverwriteCurrentValues)
{
  ADDON::CAddonInfoBuilderFromDB builder;
  builder.SetId("skin.test");
  builder.SetExtensions(ADDON::CAddonType(ADDON::AddonType::SKIN));
  auto skin = std::make_shared<CTestSkin>(builder.get(), RESOLUTION_INFO{});
  CXBMCTinyXML current;
  ASSERT_TRUE(current.Parse(std::string{
      R"(<settings><setting id="LABEL" type="string">edited</setting>
         <setting id="enabled" type="bool">false</setting></settings>)"}));
  ASSERT_TRUE(skin->SettingsFromXML(current, false));
  auto& legacySettings = CSkinSettings::GetInstance();
  CXBMCTinyXML legacy;
  ASSERT_TRUE(legacy.Parse(std::string{
      R"(<settings><skinsettings><setting name="skin.test.label" type="string">old</setting>
         <setting name="skin.test.enabled" type="bool">true</setting></skinsettings></settings>)"}));
  for (int i = 0; i < 3; ++i)
  {
    ASSERT_TRUE(legacySettings.Load(legacy.RootElement()));
    legacySettings.MigrateSettings(skin);
    EXPECT_EQ("edited", skin->GetString(skin->TranslateString("label")));
    EXPECT_FALSE(skin->GetBool(skin->TranslateBool("enabled")));
  }
  legacySettings.Clear();
}
