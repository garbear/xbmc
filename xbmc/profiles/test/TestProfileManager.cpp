/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "utils/URIUtils.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

class TestProfileManager : public testing::Test
{
protected:
  void SetUp() override
  {
    m_previousComponent = CServiceBroker::GetSettingsComponent();
    m_previousMasterPath = CSpecialProtocol::TranslatePath("special://masterprofile/");
    m_previousProfilePath = CSpecialProtocol::TranslatePath("special://profile/");
    m_directory = CSpecialProtocol::TranslatePath("special://temp/profile-settings/");
    ASSERT_TRUE(XFILE::CDirectory::Create(m_directory));
    CSpecialProtocol::SetMasterProfilePath(m_directory);
    m_component = std::make_shared<CSettingsComponent>();
    CServiceBroker::RegisterSettingsComponent(m_component);

    m_setting = std::make_shared<CSettingBool>("test.value", nullptr);
    auto* manager = m_component->GetSettings()->GetSettingsManager();
    ASSERT_TRUE(manager->AddSetting(m_setting, std::make_shared<CSettingSection>("test"),
                                    std::make_shared<CSettingCategory>("test"),
                                    std::make_shared<CSettingGroup>("test")));
    manager->SetInitialized();
  }

  void TearDown() override
  {
    CServiceBroker::RegisterSettingsComponent(m_previousComponent);
    CSpecialProtocol::SetMasterProfilePath(m_previousMasterPath);
    CSpecialProtocol::SetProfilePath(m_previousProfilePath);
    XFILE::CDirectory::RemoveRecursive(m_directory);
  }

  void Write(const std::string& path, const std::string& xml)
  {
    ASSERT_TRUE(XFILE::CDirectory::Create(URIUtils::GetDirectory(path)));
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(path, true));
    ASSERT_EQ(static_cast<ssize_t>(xml.size()), file.Write(xml.data(), xml.size()));
  }

  std::string Read(const std::string& path) const
  {
    XFILE::CFile file;
    if (!file.Open(path))
      return {};
    std::string xml(file.GetLength(), '\0');
    file.Read(xml.data(), xml.size());
    return xml;
  }

  void RejectFutureSettings(unsigned int current, unsigned int target)
  {
    const std::string profilesFile = "special://masterprofile/profiles.xml";
    Write(profilesFile, "<profiles><lastloaded>" + std::to_string(current) +
                            "</lastloaded><profile><id>0</id><name>Master</name>"
                            "<directory>special://masterprofile/</directory></profile>"
                            "<profile><id>1</id><name>One</name>"
                            "<directory>profiles/one/</directory></profile>"
                            "<profile><id>2</id><name>Two</name>"
                            "<directory>profiles/two/</directory></profile></profiles>");
    const auto profiles = m_component->GetProfileManager();
    ASSERT_TRUE(profiles->Load());
    ASSERT_EQ(current, profiles->GetCurrentProfileIndex());

    const auto settings = m_component->GetSettings();
    const std::string currentFile = profiles->GetSettingsFile();
    Write(currentFile, "<settings version=\"" + std::to_string(CSettingsManager::Version) +
                           "\"><setting id=\"test.value\">true</setting></settings>");
    ASSERT_TRUE(settings->Load());
    settings->SetLoaded();
    ASSERT_TRUE(m_setting->GetValue());

    const std::string targetFile = target == 0 ? "special://masterprofile/guisettings.xml"
                                               : "special://masterprofile/profiles/" +
                                                     std::string(target == 1 ? "one" : "two") +
                                                     "/guisettings.xml";
    const std::string futureXml = "<settings version=\"" +
                                  std::to_string(CSettingsManager::Version + 1) +
                                  "\"><setting id=\"future.value\" "
                                  "future=\"yes\">keep</setting><future-section /></settings>";
    Write(targetFile, futureXml);
    const std::string currentXml = Read(currentFile);
    const std::string profilesXml = Read(profilesFile);
    const std::string profilePath = CSpecialProtocol::TranslatePath("special://profile/");

    EXPECT_FALSE(profiles->LoadProfile(target));
    EXPECT_EQ(current, profiles->GetCurrentProfileIndex());
    EXPECT_TRUE(settings->IsLoaded());
    EXPECT_TRUE(m_setting->GetValue());
    EXPECT_EQ(currentXml, Read(currentFile));
    EXPECT_EQ(futureXml, Read(targetFile));
    EXPECT_EQ(profilesXml, Read(profilesFile));
    EXPECT_EQ(profilePath, CSpecialProtocol::TranslatePath("special://profile/"));
  }

  std::shared_ptr<CSettingsComponent> m_previousComponent;
  std::shared_ptr<CSettingsComponent> m_component;
  std::shared_ptr<CSettingBool> m_setting;
  std::string m_previousMasterPath;
  std::string m_previousProfilePath;
  std::string m_directory;
};

TEST_F(TestProfileManager, RejectsFutureProfileFromMaster)
{
  RejectFutureSettings(0, 1);
}

TEST_F(TestProfileManager, RejectsFutureMasterFromProfile)
{
  RejectFutureSettings(1, 0);
}

TEST_F(TestProfileManager, RejectsFutureProfileFromAnotherProfile)
{
  RejectFutureSettings(1, 2);
}

TEST_F(TestProfileManager, RejectsFutureProfileReload)
{
  RejectFutureSettings(1, 1);
}

TEST_F(TestProfileManager, RejectsFutureMasterReload)
{
  RejectFutureSettings(0, 0);
}
