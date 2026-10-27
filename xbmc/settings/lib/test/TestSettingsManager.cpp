/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/SettingsValueXmlSerializer.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "threads/Event.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include <gtest/gtest.h>

struct SettingIdentifierParseTest
{
  std::string_view settingId;
  std::string_view category;
  std::string_view setting;
};

std::ostream& operator<<(std::ostream& os, const SettingIdentifierParseTest& rhs)
{
  return os << rhs.settingId;
}

class TestParseSettingIdentifier : public testing::WithParamInterface<SettingIdentifierParseTest>,
                                   public testing::Test
{
};

SettingIdentifierParseTest SettingIdentifierParseTests[] = {
    {"categorytag.settingtag", "categorytag", "settingtag"},
    {"categorytag.settingtag.subsetting", "categorytag", "settingtag.subsetting"},
    {"test", "", "test"},
};

TEST_P(TestParseSettingIdentifier, Parse)
{
  const auto& params = GetParam();

  std::string categoryTag = "foo";
  std::string settingTag = "bar";
  EXPECT_TRUE(CSettingsManager::ParseSettingIdentifier(params.settingId, categoryTag, settingTag));
  EXPECT_EQ(params.category, categoryTag);
  EXPECT_EQ(params.setting, settingTag);
}

INSTANTIATE_TEST_SUITE_P(TestSettingsManager,
                         TestParseSettingIdentifier,
                         testing::ValuesIn(SettingIdentifierParseTests));

TEST(TestSettingsManager, ParseInvalidSettingIdentifier)
{
  std::string categoryTag = "foo";
  std::string settingTag = "bar";
  EXPECT_FALSE(CSettingsManager::ParseSettingIdentifier("", categoryTag, settingTag));
  EXPECT_FALSE(CSettingsManager::ParseSettingIdentifier(".test", categoryTag, settingTag));
  EXPECT_EQ("foo", categoryTag);
  EXPECT_EQ("bar", settingTag);
}

struct LocateSettingTest
{
  std::string_view m_document;
  std::string_view m_settingId;
  // nullopt means we expect the element to not be found
  std::optional<std::string_view> m_expectedSerialization;
};

class TestLocateSetting : public testing::WithParamInterface<LocateSettingTest>,
                          public testing::Test
{
};

LocateSettingTest LocateSettingTests[] = {
    // V1 format
    {
        R"(<settings><foo><bar>value</bar></foo></settings>)",
        "foo.bar",
        R"(<bar>value</bar>)",
    },
    {
        R"(<settings><foo><bar>value</bar></foo></settings>)",
        "foo",
        R"(<foo><bar>value</bar></foo>)",
    },
    {
        R"(<settings><foo><bar.baz>value</bar.baz></foo></settings>)",
        "foo.bar.baz",
        R"(<bar.baz>value</bar.baz>)",
    },
    {
        R"(<settings><foo><bar></bar></foo></settings>)",
        "foo.bar",
        R"(<bar />)",
    },
    {
        R"(<settings><foo><bar /></foo></settings>)",
        "foo.bar",
        R"(<bar />)",
    },
    {
        R"(<settings><foo><baz>value</baz></foo></settings>)",
        "foo.bar",
        std::nullopt,
    },
    {
        R"(<settings><foo></foo></settings>)",
        "foo.bar",
        std::nullopt,
    },
    {
        R"(<settings><foo /></settings>)",
        "foo.bar",
        std::nullopt,
    },

    // V2 format
    {
        R"(<settings version="2"><setting id="foo.bar">value</setting></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar">value</setting>)",
    },
    {
        R"(<settings version="2"><setting id="foo.bar"></setting></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar" />)",
    },
    {
        R"(<settings version="2"><setting id="foo.bar" /></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar" />)",
    },
    {
        R"(<settings version="2"><setting id="foo.bar"></setting><setting id="foo.baz"></setting></settings>)",
        "foo.baz",
        R"(<setting id="foo.baz" />)",
    },
    {
        R"(<settings version="2"><setting ab="foo.bar">value</setting></settings>)",
        "foo.bar",
        std::nullopt,
    },

    // Mixed V1+V2: V2 takes priority regardless of document order
    {
        R"(<settings version="2"><setting id="foo.bar">v2</setting><foo><bar>v1</bar></foo></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar">v2</setting>)",
    },
    {
        R"(<settings version="2"><foo><bar>v1</bar></foo><setting id="foo.bar">v2</setting></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar">v2</setting>)",
    },
};

TEST_P(TestLocateSetting, Locate)
{
  const auto& params = GetParam();

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{params.m_document}));

  TiXmlElement* elem = CSettingsManager::LocateSetting(doc.RootElement(), params.m_settingId);

  if (params.m_expectedSerialization.has_value())
  {
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(params.m_expectedSerialization.value(),
              XMLUtils::NodeStringSerialization(elem, XMLUtils::SerializationFormat::COMPACT));
  }
  else
  {
    EXPECT_EQ(nullptr, elem);
  }
}

INSTANTIATE_TEST_SUITE_P(TestSettingsManager,
                         TestLocateSetting,
                         testing::ValuesIn(LocateSettingTests));

TEST(TestSettingsManager, LocateSettingNull)
{
  TiXmlElement* root = nullptr;
  TiXmlElement* elem = CSettingsManager::LocateSetting(root, "foo.bar");
  EXPECT_EQ(nullptr, elem);

  const std::string document =
      R"(<settings version="2">
           <setting id="foo.bar">v2</setting>
         </settings>)";

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(document));

  elem = CSettingsManager::LocateSetting(doc.RootElement(), "");
  EXPECT_EQ(nullptr, elem);
}

namespace
{
class CConcurrentReadCallback : public ISettingCallback
{
public:
  std::shared_ptr<CSettingBool> m_setting;
  std::atomic<bool> m_readCompleted{false};

  bool OnSettingChanging(const std::shared_ptr<const CSetting>& setting) override
  {
    const auto s = m_setting;
    const auto done = std::make_shared<std::atomic<bool>>(false);

    std::thread(
        [s, done]()
        {
          (void)s->GetValue(); // shared_lock on the setting whose value is being changed
          done->store(true);
        })
        .detach();

    // Bounded, so a regression fails the test rather than hanging it
    for (int i = 0; i < 200 && !done->load(); ++i)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

    m_readCompleted.store(done->load());
    return true;
  }
};
} // namespace

// A setting change must not prevent another thread from reading the same setting while the
// OnSettingChanging handler runs. Regression test for xbmc/xbmc#20371 (JSON-RPC deadlock when
// setting videoscreen.blankdisplays).
TEST(TestSettingsManager, ValueIsReadableFromAnotherThreadDuringOnSettingChanging)
{
  const auto setting = std::make_shared<CSettingBool>("test", nullptr);
  CConcurrentReadCallback callback;
  callback.m_setting = setting;
  setting->SetCallback(&callback);

  setting->SetValue(true);

  EXPECT_TRUE(callback.m_readCompleted.load())
      << "the setting could not be read from another thread while OnSettingChanging ran - "
         "SetValue held the setting's exclusive lock across the callback (xbmc/xbmc#20371)";
}

namespace
{
// Holds the first change in its callback until released, then rejects it. Later changes,
// including the call that announces the rejected change's revert, are accepted.
class CHoldThenRejectCallback : public ISettingCallback
{
public:
  CEvent m_entered;
  CEvent m_release;

  bool OnSettingChanging(const std::shared_ptr<const CSetting>& setting) override
  {
    if (m_calls++ != 0)
      return true;

    m_entered.Set();
    m_release.Wait(std::chrono::seconds(5));
    return false;
  }

private:
  std::atomic<int> m_calls{0};
};
} // namespace

// A write of the same value made while another change's callbacks run must not be reported as
// done and then lost when that change is rejected.
TEST(TestSettingsManager, AWriteDuringARejectedChangeIsNotLost)
{
  const auto setting = std::make_shared<CSettingInt>("test", nullptr);
  CHoldThenRejectCallback callback;
  setting->SetCallback(&callback);

  std::atomic<bool> firstResult{true};
  std::atomic<bool> secondResult{false};
  std::thread first([&]() { firstResult = setting->SetValue(1); });
  const bool entered{callback.m_entered.Wait(std::chrono::seconds(5))};
  std::thread second([&]() { secondResult = setting->SetValue(1); });

  // Long enough for the second write to finish if it does not wait for the first
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  callback.m_release.Set();
  first.join();
  second.join();

  ASSERT_TRUE(entered);
  EXPECT_FALSE(firstResult);
  EXPECT_TRUE(secondResult);
  EXPECT_EQ(1, setting->GetValue()) << "the second write reported success but did not survive";
}

namespace
{
std::string ValuesXml(uint32_t version, const std::string& values)
{
  return "<settings version=\"" + std::to_string(version) + "\" future=\"keep\">" + values +
         "<future-section><value>untouched</value></future-section></settings>";
}

class TestVersionedSettings : public testing::Test
{
protected:
  void SetUp() override
  {
    m_value = std::make_shared<CSettingBool>("test.value", nullptr);
    ASSERT_TRUE(m_manager.AddSetting(m_value, std::make_shared<CSettingSection>("test"),
                                     std::make_shared<CSettingCategory>("test"),
                                     std::make_shared<CSettingGroup>("test")));
    m_manager.SetInitialized();
  }

  bool Load(const std::string& values, uint32_t version = CSettingsManager::Version + 1)
  {
    CXBMCTinyXML xml;
    if (!xml.Parse(ValuesXml(version, values)))
      return false;
    return m_manager.Load(xml.RootElement(), m_updated);
  }

  void Save(CXBMCTinyXML& xml)
  {
    CSettingsValueXmlSerializer serializer;
    std::string values;
    ASSERT_TRUE(m_manager.Save(&serializer, values));
    xml.Clear();
    ASSERT_TRUE(xml.Parse(values));
  }

  std::string Legacy(const std::string& value) const
  {
    return "<setting id=\"test.value\" version=\"" + std::to_string(CSettingsManager::Version) +
           "\">" + value + "</setting>";
  }

  CSettingsManager m_manager;
  bool m_updated{false};
  std::shared_ptr<CSettingBool> m_value;
};
} // namespace

TEST_F(TestVersionedSettings, LoadsFutureFileAndPreservesUnknownContent)
{
  ASSERT_TRUE(Load(R"(<setting id="test.value" extra="keep">true</setting>
                     <setting id="future.setting" extra="keep">future</setting>)"));
  EXPECT_TRUE(m_value->GetValue());
  CXBMCTinyXML saved;
  Save(saved);
  EXPECT_EQ(CSettingsManager::Version + 1, m_manager.ParseVersion(saved.RootElement()));
  EXPECT_STREQ("keep", saved.RootElement()->Attribute("future"));
  EXPECT_STREQ("keep", saved.RootElement()->FirstChildElement("setting")->Attribute("extra"));
  EXPECT_NE(nullptr, CSettingsManager::LocateSetting(saved.RootElement(), "future.setting"));
  EXPECT_NE(nullptr, saved.RootElement()->FirstChildElement("future-section"));
  EXPECT_EQ(nullptr, saved.RootElement()
                         ->FirstChildElement("setting")
                         ->NextSiblingElement("setting")
                         ->NextSiblingElement("setting"));
}

TEST_F(TestVersionedSettings, ExactLegacyWinsInEitherNodeOrder)
{
  const std::string current = R"(<setting id="test.value">false</setting>)";
  for (const auto& values : {current + Legacy("true"), Legacy("true") + current})
  {
    ASSERT_TRUE(Load(values));
    EXPECT_TRUE(m_value->GetValue());
  }
}

TEST_F(TestVersionedSettings, CurrentRootPrefersUnversioned)
{
  ASSERT_TRUE(Load(Legacy("true") + R"(<setting id="test.value">false</setting>)",
                   CSettingsManager::Version));
  EXPECT_FALSE(m_value->GetValue());
}

TEST_F(TestVersionedSettings, IgnoresExplicitFutureRepresentation)
{
  ASSERT_TRUE(Load("<setting id=\"test.value\" version=\"" +
                   std::to_string(CSettingsManager::Version + 1) + "\">true</setting>"));
  EXPECT_FALSE(m_value->GetValue());
}

TEST_F(TestVersionedSettings, FallsBackOnlyToUnversionedValue)
{
  ASSERT_TRUE(Load("<setting id=\"test.value\" version=\"" +
                   std::to_string(CSettingsManager::Version - 1) +
                   "\">false</setting><setting id=\"test.value\">true</setting>"));
  EXPECT_TRUE(m_value->GetValue());
}

TEST_F(TestVersionedSettings, InvalidAndMissingValuesArePreserved)
{
  ASSERT_TRUE(Load(R"(<setting id="test.value">invalid</setting>)"));
  EXPECT_FALSE(m_value->GetValue());
  CXBMCTinyXML saved;
  Save(saved);
  EXPECT_STREQ("invalid", saved.RootElement()->FirstChildElement("setting")->GetText());
  ASSERT_TRUE(Load(""));
  Save(saved);
  EXPECT_EQ(nullptr, saved.RootElement()->FirstChildElement("setting"));
}

TEST_F(TestVersionedSettings, ChangedDefaultDoesNotRewriteUnchangedValue)
{
  m_value->SetDefault(true);
  ASSERT_TRUE(Load(R"(<setting id="test.value" default="true">false</setting>)"));
  EXPECT_TRUE(m_value->GetValue());
  CXBMCTinyXML saved;
  Save(saved);
  EXPECT_STREQ("false", saved.RootElement()->FirstChildElement("setting")->GetText());
  EXPECT_EQ(nullptr, saved.RootElement()->FirstChildElement("setting")->Attribute("version"));
}

TEST_F(TestVersionedSettings, OlderEditsDoNotOverwriteCurrentRepresentation)
{
  ASSERT_TRUE(Load(R"(<setting id="test.value" extra="keep">invalid-for-bool</setting>)"));
  ASSERT_TRUE(m_value->SetValue(true));
  CXBMCTinyXML saved;
  for (int i = 0; i < 3; ++i)
  {
    Save(saved);
    const auto* current = saved.RootElement()->FirstChildElement("setting");
    ASSERT_NE(nullptr, current);
    EXPECT_STREQ("invalid-for-bool", current->GetText());
    const auto* legacy = current->NextSiblingElement("setting");
    ASSERT_NE(nullptr, legacy);
    EXPECT_STREQ("true", legacy->GetText());
    EXPECT_EQ(nullptr, legacy->NextSiblingElement("setting"));
    bool updated = false;
    ASSERT_TRUE(m_manager.Load(saved.RootElement(), updated));
    EXPECT_TRUE(m_value->GetValue());
  }
}

TEST_F(TestVersionedSettings, EditsExactLegacyWithoutTouchingCurrent)
{
  ASSERT_TRUE(Load(Legacy("false") + R"(<setting id="test.value">42</setting>)"));
  ASSERT_TRUE(m_value->SetValue(true));
  CXBMCTinyXML saved;
  Save(saved);
  const auto* legacy = saved.RootElement()->FirstChildElement("setting");
  ASSERT_NE(nullptr, legacy);
  EXPECT_STREQ("true", legacy->GetText());
  const auto* current = legacy->NextSiblingElement("setting");
  ASSERT_NE(nullptr, current);
  EXPECT_STREQ("42", current->GetText());
  EXPECT_EQ(nullptr, current->Attribute("version"));
}

TEST_F(TestVersionedSettings, CurrentEditsPreserveUnknownAttributesAndChildren)
{
  ASSERT_TRUE(Load(R"(<setting id="test.value" extra="keep">false<future /></setting>)",
                   CSettingsManager::Version));
  ASSERT_TRUE(m_value->SetValue(true));
  CXBMCTinyXML saved;
  Save(saved);
  const auto* value = saved.RootElement()->FirstChildElement("setting");
  EXPECT_STREQ("true", value->GetText());
  EXPECT_STREQ("keep", value->Attribute("extra"));
  EXPECT_NE(nullptr, value->FirstChildElement("future"));
}

TEST_F(TestVersionedSettings, V1ValuesAndSectionsSurvive)
{
  ASSERT_TRUE(Load(R"(<test extra="keep"><value>true</value><unknown>keep</unknown></test>)", 0));
  EXPECT_TRUE(m_value->GetValue());
  CXBMCTinyXML saved;
  Save(saved);
  EXPECT_EQ(CSettingsManager::Version, m_manager.ParseVersion(saved.RootElement()));
  ASSERT_NE(nullptr, saved.RootElement()->FirstChildElement("test"));
  EXPECT_NE(nullptr, saved.RootElement()->FirstChildElement("test")->FirstChildElement("unknown"));
}

TEST_F(TestVersionedSettings, DefinitionVersionsRemainStrict)
{
  CSettingsManager manager;
  CXBMCTinyXML xml;
  ASSERT_TRUE(xml.Parse(ValuesXml(CSettingsManager::Version + 1, "")));
  EXPECT_FALSE(manager.Initialize(xml.RootElement()));
}

namespace
{
class CUpdateSettingCallback : public ISettingCallback
{
public:
  bool OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                       const char*,
                       const TiXmlNode*) override
  {
    ++m_calls;
    if (m_dependent)
      m_dependent->SetValue(true);
    return setting->FromString("true");
  }
  int m_calls{0};
  std::shared_ptr<CSettingBool> m_dependent;
};
} // namespace

TEST_F(TestVersionedSettings, LegacyConversionsAreSavedWithoutRewritingDefaultNormalization)
{
  CXBMCTinyXML definition;
  ASSERT_TRUE(definition.Parse(std::string{
      R"(<setting id="test.value"><updates><update type="change" /></updates></setting>)"}));
  ASSERT_TRUE(m_value->Deserialize(definition.RootElement(), true));
  CUpdateSettingCallback callback;
  m_manager.RegisterCallback(&callback, {"test.value"});
  auto normalized = std::make_shared<CSettingBool>("test.normalized", nullptr);
  normalized->SetDefault(true);
  ASSERT_TRUE(m_manager.AddSetting(normalized, std::make_shared<CSettingSection>("normalized"),
                                   std::make_shared<CSettingCategory>("test"),
                                   std::make_shared<CSettingGroup>("test")));
  ASSERT_TRUE(Load(R"(<setting id="test.value" extra="keep">false</setting>
                     <setting id="test.normalized" default="true">false</setting>
                     <setting id="future.unknown" extra="keep">unknown</setting>)",
                   CSettingsManager::Version));
  EXPECT_TRUE(m_updated);
  EXPECT_TRUE(normalized->GetValue());
  EXPECT_EQ(1, callback.m_calls);
  EXPECT_TRUE(m_value->GetValue());
  CXBMCTinyXML saved;
  Save(saved);
  const auto* converted = CSettingsManager::LocateSetting(saved.RootElement(), "test.value");
  ASSERT_NE(nullptr, converted);
  EXPECT_STREQ("true", converted->GetText());
  EXPECT_STREQ("keep", converted->Attribute("extra"));
  EXPECT_STREQ("false",
               CSettingsManager::LocateSetting(saved.RootElement(), "test.normalized")->GetText());
  EXPECT_STREQ("unknown",
               CSettingsManager::LocateSetting(saved.RootElement(), "future.unknown")->GetText());
  bool updated = false;
  ASSERT_TRUE(m_manager.Load(saved.RootElement(), updated));
  EXPECT_FALSE(updated);
  EXPECT_TRUE(m_value->GetValue());
  m_manager.UnregisterCallback(&callback);
}

TEST_F(TestVersionedSettings, SuccessfulNoOpCallbackDoesNotWriteOrResetDefault)
{
  CXBMCTinyXML definition;
  ASSERT_TRUE(definition.Parse(std::string{
      R"(<setting id="test.value"><updates><update type="change" /></updates></setting>)"}));
  ASSERT_TRUE(m_value->Deserialize(definition.RootElement(), true));
  CUpdateSettingCallback callback;
  m_manager.RegisterCallback(&callback, {"test.value"});
  ASSERT_TRUE(
      Load(R"(<setting id="test.value" default="true">true</setting>)", CSettingsManager::Version));
  EXPECT_FALSE(m_updated);
  EXPECT_TRUE(m_value->GetValue());
  EXPECT_FALSE(m_manager.HasSettingChanged("test.value"));
  CXBMCTinyXML saved;
  Save(saved);
  EXPECT_STREQ("true", saved.RootElement()->FirstChildElement("setting")->Attribute("default"));
  m_manager.UnregisterCallback(&callback);
}

TEST_F(TestVersionedSettings, LegacyCallbackPersistsDependentSettingChanges)
{
  CXBMCTinyXML definition;
  ASSERT_TRUE(definition.Parse(std::string{
      R"(<setting id="test.value"><updates><update type="change" /></updates></setting>)"}));
  ASSERT_TRUE(m_value->Deserialize(definition.RootElement(), true));
  CUpdateSettingCallback callback;
  callback.m_dependent = std::make_shared<CSettingBool>("test.dependent", nullptr);
  ASSERT_TRUE(m_manager.AddSetting(
      callback.m_dependent, std::make_shared<CSettingSection>("dependent"),
      std::make_shared<CSettingCategory>("test"), std::make_shared<CSettingGroup>("test")));
  m_manager.RegisterCallback(&callback, {"test.value"});
  ASSERT_TRUE(Load(R"(<setting id="test.dependent" extra="keep">false</setting>
                     <setting id="test.value">true</setting>)",
                   CSettingsManager::Version));
  EXPECT_TRUE(m_updated);
  CXBMCTinyXML saved;
  Save(saved);
  const auto* dependent = CSettingsManager::LocateSetting(saved.RootElement(), "test.dependent");
  ASSERT_NE(nullptr, dependent);
  EXPECT_STREQ("true", dependent->GetText());
  EXPECT_STREQ("keep", dependent->Attribute("extra"));
  bool updated = false;
  ASSERT_TRUE(m_manager.Load(saved.RootElement(), updated));
  EXPECT_FALSE(updated);
  EXPECT_TRUE(callback.m_dependent->GetValue());
  m_manager.UnregisterCallback(&callback);
}

TEST_F(TestVersionedSettings, LegacyCallbackOnlyChangesMatchingVersionedRepresentation)
{
  CXBMCTinyXML definition;
  ASSERT_TRUE(definition.Parse(std::string{
      R"(<setting id="test.value"><updates><update type="change" /></updates></setting>)"}));
  ASSERT_TRUE(m_value->Deserialize(definition.RootElement(), true));
  CUpdateSettingCallback callback;
  m_manager.RegisterCallback(&callback, {"test.value"});
  ASSERT_TRUE(Load(R"(<setting id="test.value" extra="keep">future</setting>)" + Legacy("false")));
  EXPECT_TRUE(m_updated);
  CXBMCTinyXML saved;
  Save(saved);
  const auto* current = saved.RootElement()->FirstChildElement("setting");
  EXPECT_STREQ("future", current->GetText());
  EXPECT_STREQ("keep", current->Attribute("extra"));
  const auto* legacy = CSettingsManager::LocateSetting(saved.RootElement(), "test.value");
  EXPECT_STREQ("true", legacy->GetText());
  EXPECT_STREQ(std::to_string(CSettingsManager::Version).c_str(), legacy->Attribute("version"));
  m_manager.UnregisterCallback(&callback);
}

TEST_F(TestVersionedSettings, DoesNotApplyOlderUpdateCallbacksToFutureValues)
{
  CXBMCTinyXML definition;
  ASSERT_TRUE(definition.Parse(std::string{
      R"(<setting id="test.value"><updates><update type="change" /></updates></setting>)"}));
  ASSERT_TRUE(m_value->Deserialize(definition.RootElement(), true));
  CUpdateSettingCallback callback;
  m_manager.RegisterCallback(&callback, {"test.value"});
  ASSERT_TRUE(Load(R"(<setting id="test.value">false</setting>)"));
  EXPECT_EQ(0, callback.m_calls);
  EXPECT_FALSE(m_value->GetValue());
  m_manager.UnregisterCallback(&callback);
}

TEST_F(TestVersionedSettings, NewFilesSerializeDefaults)
{
  CXBMCTinyXML saved;
  Save(saved);
  EXPECT_EQ(CSettingsManager::Version, m_manager.ParseVersion(saved.RootElement()));
  const auto* value = saved.RootElement()->FirstChildElement("setting");
  ASSERT_NE(nullptr, value);
  EXPECT_STREQ("false", value->GetText());
  EXPECT_STREQ("true", value->Attribute("default"));
  EXPECT_EQ(nullptr, value->Attribute("version"));
}

TEST_F(TestVersionedSettings, V1FallbackDoesNotLoadExplicitFutureValue)
{
  ASSERT_TRUE(Load("<test><value version=\"" + std::to_string(CSettingsManager::Version + 1) +
                   "\">true</value></test>"));
  EXPECT_FALSE(m_value->GetValue());
}
