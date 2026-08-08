/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CompileInfo.h"
#include "addons/AddonBuilder.h"
#include "addons/PluginSource.h"
#include "addons/Repository.h"
#include "addons/ServiceCatalog.h"
#include "addons/ServiceManifest.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "utils/XBMCTinyXML2.h"

#include <set>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

using namespace ADDON;

const std::string addonXML = R"xml(
<addon id="metadata.blablabla.org"
       name="The Bla Bla Bla Addon"
       version="1.2.3"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
    <import addon="metadata.common.imdb.com" minversion="2.9.2" version="2.9.2"/>
    <import addon="metadata.common.themoviedb.org" minversion="3.1.0" version="3.1.0"/>
    <import addon="plugin.video.youtube" minversion="4.4.0" version="4.4.10" optional="true"/>
  </requires>
  <extension point="xbmc.metadata.scraper.movies"
             language="en"
             library="blablabla.xml"/>
  <extension point="xbmc.python.module"
             library="lib.so"/>
  <extension point="kodi.addon.metadata">
    <summary lang="en">Summary bla bla bla</summary>
    <description lang="en">Description bla bla bla</description>
    <disclaimer lang="en">Disclaimer bla bla bla</disclaimer>
    <platform>all</platform>
    <language>marsian</language>
    <license>GPL v2.0</license>
    <forum>https://forum.kodi.tv</forum>
    <website>https://kodi.tv</website>
    <email>a@a.dummy</email>
    <source>https://github.com/xbmc/xbmc</source>
  </extension>
</addon>
)xml";

namespace
{
constexpr std::string_view CINEMATIC_EARTH_MANIFEST_URI =
    "data:application/json;base64,"
    "eyJ2ZXJzaW9uIjoxLCJpZCI6ImNpbmVtYXRpYy5lYXJ0aCIsIm5hbWUiOiJjaW5lbWF0aWMu"
    "ZWFydGgifQ==";

AddonInfoPtr GenerateWithLibrary(const std::string& libraryName)
{
  const std::string xml = R"xml(
<addon id="binary.blablabla.org"
       name="The Binary Bla Bla Bla Addon"
       version="1.2.3"
       provider-name="Team Kodi">
  <extension point="xbmc.python.module" library=")xml" +
                          libraryName + R"xml("/>
  <extension point="kodi.addon.metadata">
    <platform>all</platform>
  </extension>
</addon>
)xml";

  CXBMCTinyXML2 doc;
  EXPECT_TRUE(doc.Parse(xml));
  return CAddonInfoBuilder::Generate(doc.RootElement(), RepositoryDirInfo{});
}

std::shared_ptr<CPluginSource> GeneratePluginSource(const std::string& manifestAttribute)
{
  const std::string xml = R"xml(
<addon id="plugin.video.example"
       name="Example Video Add-on"
       version="1.0.0"
       provider-name="Team Kodi">
  <extension point="xbmc.python.pluginsource"
             library="resources/lib/plugin.py")xml" +
                          manifestAttribute + R"xml(>
    <provides>video</provides>
  </extension>
</addon>
)xml";

  CXBMCTinyXML2 doc;
  EXPECT_TRUE(doc.Parse(xml));
  const AddonInfoPtr addonInfo =
      CAddonInfoBuilder::Generate(doc.RootElement(), RepositoryDirInfo{});
  return std::dynamic_pointer_cast<CPluginSource>(
      CAddonBuilder::Generate(addonInfo, AddonType::PLUGIN));
}
} // namespace

class TestAddonInfoBuilder : public ::testing::Test
{
protected:
  TestAddonInfoBuilder() = default;
};

TEST_F(TestAddonInfoBuilder, ShouldFailWhenIdIsNotSet)
{
  AddonInfoPtr addon = CAddonInfoBuilder::Generate("", AddonType::UNKNOWN);
  EXPECT_EQ(nullptr, addon);
}

TEST_F(TestAddonInfoBuilder, TestGenerate_Id_Type)
{
  AddonInfoPtr addon = CAddonInfoBuilder::Generate("foo.baz", AddonType::VISUALIZATION);
  EXPECT_NE(nullptr, addon);
  EXPECT_EQ(addon->ID(), "foo.baz");
  EXPECT_EQ(addon->MainType(), AddonType::VISUALIZATION);
  EXPECT_TRUE(addon->HasType(AddonType::VISUALIZATION));
  EXPECT_FALSE(addon->HasType(AddonType::SCREENSAVER));
}

TEST_F(TestAddonInfoBuilder, TestGenerate_Repo)
{
  CXBMCTinyXML2 doc;
  EXPECT_TRUE(doc.Parse(addonXML));
  ASSERT_NE(nullptr, doc.RootElement());

  RepositoryDirInfo repo;
  AddonInfoPtr addon = CAddonInfoBuilder::Generate(doc.RootElement(), repo);
  ASSERT_NE(nullptr, addon);
  EXPECT_EQ(addon->ID(), "metadata.blablabla.org");

  EXPECT_EQ(addon->MainType(), AddonType::SCRAPER_MOVIES);
  EXPECT_TRUE(addon->HasType(AddonType::SCRAPER_MOVIES));
  EXPECT_EQ(addon->Type(AddonType::SCRAPER_MOVIES)->LibName(), "blablabla.xml");
  EXPECT_EQ(addon->Type(AddonType::SCRAPER_MOVIES)->GetValue("@language").asString(), "en");

  EXPECT_TRUE(addon->HasType(AddonType::SCRIPT_MODULE));
  EXPECT_EQ(addon->Type(AddonType::SCRIPT_MODULE)->LibName(), "lib.so");
  EXPECT_FALSE(addon->HasType(AddonType::SCRAPER_ARTISTS));

  EXPECT_EQ(addon->Name(), "The Bla Bla Bla Addon");
  EXPECT_EQ(addon->Author(), "Team Kodi");
  EXPECT_EQ(addon->Version().asString(), "1.2.3");

  EXPECT_EQ(addon->Summary(), "Summary bla bla bla");
  EXPECT_EQ(addon->Description(), "Description bla bla bla");
  EXPECT_EQ(addon->Disclaimer(), "Disclaimer bla bla bla");
  EXPECT_EQ(addon->License(), "GPL v2.0");
  EXPECT_EQ(addon->Forum(), "https://forum.kodi.tv");
  EXPECT_EQ(addon->Website(), "https://kodi.tv");
  EXPECT_EQ(addon->EMail(), "a@a.dummy");
  EXPECT_EQ(addon->Source(), "https://github.com/xbmc/xbmc");

  const std::vector<DependencyInfo>& dependencies = addon->GetDependencies();
  ASSERT_EQ(dependencies.size(), (long unsigned int)4);
  EXPECT_EQ(dependencies[0].id, "xbmc.metadata");
  EXPECT_EQ(dependencies[0].optional, false);
  EXPECT_EQ(dependencies[0].versionMin.asString(), "2.1.0");
  EXPECT_EQ(dependencies[0].version.asString(), "2.1.0");
  EXPECT_EQ(dependencies[1].id, "metadata.common.imdb.com");
  EXPECT_EQ(dependencies[1].optional, false);
  EXPECT_EQ(dependencies[1].versionMin.asString(), "2.9.2");
  EXPECT_EQ(dependencies[1].version.asString(), "2.9.2");
  EXPECT_EQ(dependencies[2].id, "metadata.common.themoviedb.org");
  EXPECT_EQ(dependencies[2].optional, false);
  EXPECT_EQ(dependencies[2].versionMin.asString(), "3.1.0");
  EXPECT_EQ(dependencies[2].version.asString(), "3.1.0");
  EXPECT_EQ(dependencies[3].id, "plugin.video.youtube");
  EXPECT_EQ(dependencies[3].optional, true);
  EXPECT_EQ(dependencies[3].versionMin.asString(), "4.4.0");
  EXPECT_EQ(dependencies[3].version.asString(), "4.4.10");

  auto info = addon->ExtraInfo().find("language");
  ASSERT_NE(info, addon->ExtraInfo().end());
  EXPECT_EQ(info->second, "marsian");
}

TEST_F(TestAddonInfoBuilder, BinaryDetection_AcceptsPlatformSharedLibrary)
{
  const std::string suffix = CCompileInfo::GetSharedLibrarySuffix();

  EXPECT_TRUE(GenerateWithLibrary("libfoo" + suffix)->IsBinary());
  // linux is different and has the version number after the suffix
  EXPECT_TRUE(GenerateWithLibrary("libfoo" + suffix + ".1")->IsBinary());
  EXPECT_TRUE(GenerateWithLibrary("libfoo" + suffix + ".1.2.3")->IsBinary());
}

TEST_F(TestAddonInfoBuilder, BinaryDetection_RejectsNonSharedLibrary)
{
  const std::string suffix = CCompileInfo::GetSharedLibrarySuffix();
  ASSERT_TRUE(suffix.starts_with('.')); // the escaping in the regex depends on this

  // the dot of the suffix must be matched literally, not as "any character"
  EXPECT_FALSE(GenerateWithLibrary("libfooX" + suffix.substr(1))->IsBinary());
  EXPECT_FALSE(GenerateWithLibrary("libfoo_" + suffix.substr(1))->IsBinary());

  EXPECT_FALSE(GenerateWithLibrary("blablabla.xml")->IsBinary());
  EXPECT_FALSE(GenerateWithLibrary("default.py")->IsBinary());
}

TEST_F(TestAddonInfoBuilder, PluginSourceLoadsEmbeddedServiceManifest)
{
  const auto plugin =
      GeneratePluginSource(" manifest=\"" + std::string{CINEMATIC_EARTH_MANIFEST_URI} + "\"");

  ASSERT_NE(nullptr, plugin);
  EXPECT_EQ(CINEMATIC_EARTH_MANIFEST_URI, plugin->Manifest());
  EXPECT_EQ("resources/lib/plugin.py", plugin->AddonInfo()->Type(AddonType::PLUGIN)->LibName());
  EXPECT_TRUE(plugin->Provides(CPluginSource::Content::VIDEO));

  CServiceManifest manifest;
  CServiceManifest::Error error{CServiceManifest::Error::UNKNOWN};
  ASSERT_TRUE(plugin->LoadServiceManifest(manifest, &error));
  EXPECT_EQ(CServiceManifest::Error::NONE, error);
  EXPECT_EQ(1U, manifest.Version());
  EXPECT_EQ("cinematic.earth", manifest.ID());
  EXPECT_EQ("cinematic.earth", manifest.Name());
}

TEST_F(TestAddonInfoBuilder, CinematicEarthServiceCatalogIntegration)
{
  const AddonInfoPtr addonInfo =
      CAddonInfoBuilder::Generate("special://xbmc/addons/plugin.cinematic.earth");
  ASSERT_NE(nullptr, addonInfo);

  const auto plugin = std::dynamic_pointer_cast<CPluginSource>(
      CAddonBuilder::Generate(addonInfo, AddonType::PLUGIN));
  ASSERT_NE(nullptr, plugin);

  CServiceManifest manifest;
  CServiceManifest::Error manifestError{CServiceManifest::Error::UNKNOWN};
  ASSERT_TRUE(plugin->LoadServiceManifest(manifest, &manifestError))
      << "manifest URI: " << plugin->Manifest()
      << ", error: " << static_cast<int>(manifestError);
  EXPECT_EQ(CServiceManifest::Error::NONE, manifestError);
  EXPECT_EQ(1U, manifest.Version());
  EXPECT_EQ("cinematic.earth", manifest.ID());
  EXPECT_EQ("cinematic.earth", manifest.Name());
  EXPECT_EQ("special://xbmc/addons/plugin.cinematic.earth/resources/catalog.json",
            manifest.Catalog());

  CServiceCatalog catalog;
  CServiceCatalog::Error catalogError{CServiceCatalog::Error::UNKNOWN};
  ASSERT_TRUE(CServiceCatalog::Load(manifest.Catalog(), catalog, &catalogError))
      << "catalog URI: " << manifest.Catalog()
      << ", error: " << static_cast<int>(catalogError);
  EXPECT_EQ(CServiceCatalog::Error::NONE, catalogError);
  EXPECT_EQ(1U, catalog.Version());
  EXPECT_TRUE(catalog.Items().empty());
}

TEST_F(TestAddonInfoBuilder, PluginSourceWithoutManifest)
{
  const auto plugin = GeneratePluginSource("");

  ASSERT_NE(nullptr, plugin);
  EXPECT_TRUE(plugin->Manifest().empty());
  EXPECT_EQ("resources/lib/plugin.py", plugin->AddonInfo()->Type(AddonType::PLUGIN)->LibName());
  EXPECT_TRUE(plugin->Provides(CPluginSource::Content::VIDEO));

  CServiceManifest manifest;
  EXPECT_FALSE(plugin->LoadServiceManifest(manifest));
}

TEST_F(TestAddonInfoBuilder, BrokenServiceManifestDoesNotInvalidatePluginSource)
{
  constexpr std::string_view missingManifestUri = "/kodi-test-missing/plugin-service-manifest.json";
  const auto plugin = GeneratePluginSource(" manifest=\"" + std::string{missingManifestUri} + "\"");

  ASSERT_NE(nullptr, plugin);
  EXPECT_EQ(missingManifestUri, plugin->Manifest());
  EXPECT_TRUE(plugin->Provides(CPluginSource::Content::VIDEO));

  CServiceManifest manifest;
  ASSERT_TRUE(CServiceManifest::Parse(
      R"({"version":1,"id":"unchanged.example","name":"Unchanged"})", manifest));

  CServiceManifest::Error error{CServiceManifest::Error::UNKNOWN};
  EXPECT_FALSE(plugin->LoadServiceManifest(manifest, &error));
  EXPECT_EQ(CServiceManifest::Error::OPEN_FAILED, error);
  EXPECT_EQ(1U, manifest.Version());
  EXPECT_EQ("unchanged.example", manifest.ID());
  EXPECT_EQ("Unchanged", manifest.Name());
}

TEST_F(TestAddonInfoBuilder, InvalidServiceManifestDoesNotInvalidatePluginSource)
{
  constexpr std::string_view invalidManifestUri = "data:application/json,%7B%22version%22%3A1%7D";
  const auto plugin = GeneratePluginSource(" manifest=\"" + std::string{invalidManifestUri} + "\"");

  ASSERT_NE(nullptr, plugin);
  EXPECT_EQ(invalidManifestUri, plugin->Manifest());
  EXPECT_TRUE(plugin->Provides(CPluginSource::Content::VIDEO));

  CServiceManifest manifest;
  CServiceManifest::Error error{CServiceManifest::Error::UNKNOWN};
  EXPECT_FALSE(plugin->LoadServiceManifest(manifest, &error));
  EXPECT_EQ(CServiceManifest::Error::MISSING_ID, error);
}

TEST_F(TestAddonInfoBuilder, TestGenerate_DBEntry)
{
  CAddonInfoBuilderFromDB builder;
  builder.SetId("video.blablabla.org");
  builder.SetVersion(CAddonVersion("1.2.3"));
  CAddonType addonType(AddonType::PLUGIN);
  addonType.Insert("provides", "video audio");
  builder.SetExtensions(addonType);
  builder.SetName("The Bla Bla Bla Addon");
  builder.SetAuthor("Team Kodi");
  builder.SetSummary("Summary bla bla bla");
  builder.SetDescription("Description bla bla bla");
  builder.SetDisclaimer("Disclaimer bla bla bla");
  builder.SetLicense("GPL v2.0");
  builder.SetForum("https://forum.kodi.tv");
  builder.SetWebsite("https://kodi.tv");
  builder.SetEMail("a@a.dummy");
  builder.SetSource("https://github.com/xbmc/xbmc");
  InfoMap extrainfo;
  extrainfo["language"] = "marsian";
  builder.SetExtrainfo(extrainfo);

  AddonInfoPtr addon = builder.get();
  ASSERT_NE(nullptr, addon);
  EXPECT_EQ(addon->ID(), "video.blablabla.org");

  EXPECT_EQ(addon->MainType(), AddonType::PLUGIN);
  EXPECT_TRUE(addon->HasType(AddonType::PLUGIN));
  EXPECT_TRUE(addon->HasType(AddonType::VIDEO));
  EXPECT_TRUE(addon->HasType(AddonType::AUDIO));
  EXPECT_FALSE(addon->HasType(AddonType::GAME));

  EXPECT_EQ(addon->Name(), "The Bla Bla Bla Addon");
  EXPECT_EQ(addon->Author(), "Team Kodi");
  EXPECT_EQ(addon->Version().asString(), "1.2.3");

  EXPECT_EQ(addon->Summary(), "Summary bla bla bla");
  EXPECT_EQ(addon->Description(), "Description bla bla bla");
  EXPECT_EQ(addon->Disclaimer(), "Disclaimer bla bla bla");
  EXPECT_EQ(addon->License(), "GPL v2.0");
  EXPECT_EQ(addon->Forum(), "https://forum.kodi.tv");
  EXPECT_EQ(addon->Website(), "https://kodi.tv");
  EXPECT_EQ(addon->EMail(), "a@a.dummy");
  EXPECT_EQ(addon->Source(), "https://github.com/xbmc/xbmc");

  auto info = addon->ExtraInfo().find("language");
  ASSERT_NE(info, addon->ExtraInfo().end());
  EXPECT_EQ(info->second, "marsian");
}
