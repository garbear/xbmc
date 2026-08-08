/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/AddonBuilder.h"
#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "interfaces/legacy/Addon.h"
#include "utils/Base64.h"
#include "utils/XBMCTinyXML2.h"

#include <string>

#include <gtest/gtest.h>

using namespace ADDON;
using namespace XBMCAddon;
using namespace XBMCAddon::xbmcaddon;

namespace
{
std::string DataUri(const std::string& json)
{
  return "data:application/json;base64," + Base64::Encode(json);
}

AddonPtr GeneratePluginSource(const std::string& manifestUri = {})
{
  const std::string manifestAttribute =
      manifestUri.empty() ? "" : " manifest=\"" + manifestUri + "\"";
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

  CXBMCTinyXML2 document;
  EXPECT_TRUE(document.Parse(xml));
  const AddonInfoPtr addonInfo =
      CAddonInfoBuilder::Generate(document.RootElement(), RepositoryDirInfo{});
  return CAddonBuilder::Generate(addonInfo, AddonType::PLUGIN);
}

std::string ManifestUri(const std::string& catalogUri = {})
{
  std::string manifest = R"({"version":1,"id":"example","name":"Example"})";
  if (!catalogUri.empty())
  {
    manifest.pop_back();
    manifest += R"(,"catalog":")" + catalogUri + R"("})";
  }

  return DataUri(manifest);
}
} // namespace

TEST(TestPythonServiceCatalog, NoManifestReturnsNone)
{
  EXPECT_EQ(nullptr, GetServiceCatalog(GeneratePluginSource()));
}

TEST(TestPythonServiceCatalog, ManifestWithoutCatalogReturnsNone)
{
  EXPECT_EQ(nullptr, GetServiceCatalog(GeneratePluginSource(ManifestUri())));
}

TEST(TestPythonServiceCatalog, MalformedConfiguredManifestRaises)
{
  EXPECT_THROW(GetServiceCatalog(GeneratePluginSource(DataUri(R"({"version":1})"))),
               AddonException);
}

TEST(TestPythonServiceCatalog, MalformedConfiguredCatalogRaises)
{
  EXPECT_THROW(GetServiceCatalog(GeneratePluginSource(ManifestUri(DataUri("{")))), AddonException);
}

TEST(TestPythonServiceCatalog, EmptyCatalogPreservesVersion)
{
  const auto result =
      GetServiceCatalog(GeneratePluginSource(ManifestUri(DataUri(R"({"version":1,"items":[]})"))));

  ASSERT_NE(nullptr, result);
  ASSERT_EQ(2U, result->size());
  EXPECT_EQ(1U, result->at("version").former());
  EXPECT_TRUE(result->at("items").later().empty());
}

TEST(TestPythonServiceCatalog, LoadsConfiguredSpecialCatalog)
{
  const AddonInfoPtr addonInfo =
      CAddonInfoBuilder::Generate("special://xbmc/addons/plugin.cinematic.earth");
  ASSERT_NE(nullptr, addonInfo);

  const auto result = GetServiceCatalog(CAddonBuilder::Generate(addonInfo, AddonType::PLUGIN));

  ASSERT_NE(nullptr, result);
  EXPECT_EQ(1U, result->at("version").former());
  EXPECT_TRUE(result->at("items").later().empty());
}

TEST(TestPythonServiceCatalog, ReturnsOrderedItemsWithExactValues)
{
  const std::string catalog = R"({
    "version": 1,
    "items": [
      {"id":"second.id","name":"Second name","media":"special://media/second.mkv"},
      {"id":"first.id","name":" First name ","media":"data:video/mp4;base64,AA=="}
    ]
  })";
  const auto result = GetServiceCatalog(GeneratePluginSource(ManifestUri(DataUri(catalog))));

  ASSERT_NE(nullptr, result);
  EXPECT_EQ(1U, result->at("version").former());

  const auto& items = result->at("items").later();
  ASSERT_EQ(2U, items.size());
  EXPECT_EQ("second.id", items[0].at("id"));
  EXPECT_EQ("Second name", items[0].at("name"));
  EXPECT_EQ("special://media/second.mkv", items[0].at("media"));
  EXPECT_EQ("first.id", items[1].at("id"));
  EXPECT_EQ(" First name ", items[1].at("name"));
  EXPECT_EQ("data:video/mp4;base64,AA==", items[1].at("media"));
}
