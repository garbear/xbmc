/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/XMLUtils.h"
#include "utils/XBMCTinyXML.h"
#include "video/VideoInfoTag.h"

#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
std::vector<std::string> GetSavedTags(const CXBMCTinyXML& doc)
{
  std::vector<std::string> tags;

  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return tags;

  const TiXmlElement* tag = movie->FirstChildElement("tag");
  while (tag)
  {
    if (tag->FirstChild())
      tags.emplace_back(tag->FirstChild()->ValueStr());
    tag = tag->NextSiblingElement("tag");
  }

  return tags;
}

std::vector<std::string> GetSavedStrings(const CXBMCTinyXML& doc, const char* elementName)
{
  std::vector<std::string> values;

  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return values;

  const TiXmlElement* element = movie->FirstChildElement(elementName);
  while (element)
  {
    if (element->FirstChild())
      values.emplace_back(element->FirstChild()->ValueStr());
    element = element->NextSiblingElement(elementName);
  }

  return values;
}

std::vector<std::string> GetSavedActors(const CXBMCTinyXML& doc)
{
  std::vector<std::string> actors;

  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return actors;

  const TiXmlElement* actor = movie->FirstChildElement("actor");
  while (actor)
  {
    std::string name;
    std::string role;
    XMLUtils::GetString(actor, "name", name);
    XMLUtils::GetString(actor, "role", role);
    actors.emplace_back(name + ":" + role);
    actor = actor->NextSiblingElement("actor");
  }

  return actors;
}

std::vector<std::string> GetSavedActorKeys(const CXBMCTinyXML& doc)
{
  std::vector<std::string> actors;

  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return actors;

  const TiXmlElement* actor = movie->FirstChildElement("actor");
  while (actor)
  {
    std::string name;
    std::string role;
    std::string thumb;
    XMLUtils::GetString(actor, "name", name);
    XMLUtils::GetString(actor, "role", role);
    XMLUtils::GetString(actor, "thumb", thumb);
    actors.emplace_back(name + ":" + role + ":" + thumb);
    actor = actor->NextSiblingElement("actor");
  }

  return actors;
}

std::vector<std::string> GetSavedThumbKeys(const CXBMCTinyXML& doc)
{
  std::vector<std::string> thumbs;

  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return thumbs;

  const TiXmlElement* thumb = movie->FirstChildElement("thumb");
  while (thumb)
  {
    std::string season;
    if (thumb->Attribute("season"))
      season = XMLUtils::GetAttribute(thumb, "season");

    const std::string url = thumb->FirstChild() ? thumb->FirstChild()->ValueStr() : "";
    thumbs.emplace_back(XMLUtils::GetAttribute(thumb, "type") + ":" + season + ":" +
                        XMLUtils::GetAttribute(thumb, "aspect") + ":" +
                        XMLUtils::GetAttribute(thumb, "language") + ":" +
                        XMLUtils::GetAttribute(thumb, "spoof") + ":" +
                        XMLUtils::GetAttribute(thumb, "cache") + ":" + url + ":" +
                        XMLUtils::GetAttribute(thumb, "preview"));
    thumb = thumb->NextSiblingElement("thumb");
  }

  return thumbs;
}

std::vector<std::string> GetSavedFanartThumbKeys(const CXBMCTinyXML& doc)
{
  std::vector<std::string> thumbs;

  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return thumbs;

  const TiXmlElement* fanart = movie->FirstChildElement("fanart");
  if (!fanart)
    return thumbs;

  const TiXmlElement* thumb = fanart->FirstChildElement("thumb");
  while (thumb)
  {
    const std::string url = thumb->FirstChild() ? thumb->FirstChild()->ValueStr() : "";
    thumbs.emplace_back(url + ":" + XMLUtils::GetAttribute(thumb, "preview") + ":" +
                        XMLUtils::GetAttribute(thumb, "colors"));
    thumb = thumb->NextSiblingElement("thumb");
  }

  return thumbs;
}

std::string GetSavedFanartAttribute(const CXBMCTinyXML& doc, const char* attribute)
{
  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return {};

  const TiXmlElement* fanart = movie->FirstChildElement("fanart");
  if (!fanart)
    return {};

  return XMLUtils::GetAttribute(fanart, attribute);
}

bool HasSavedFanart(const CXBMCTinyXML& doc)
{
  const TiXmlElement* movie = doc.RootElement();
  return movie && movie->FirstChildElement("fanart");
}

bool HasSavedThumb(const CXBMCTinyXML& doc)
{
  const TiXmlElement* movie = doc.RootElement();
  return movie && movie->FirstChildElement("thumb");
}

std::vector<std::string> GetSavedUniqueIDKeys(const CXBMCTinyXML& doc)
{
  std::vector<std::string> keys;

  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return keys;

  const TiXmlElement* uniqueid = movie->FirstChildElement("uniqueid");
  while (uniqueid)
  {
    std::string type;
    uniqueid->QueryStringAttribute("type", &type);
    bool isDefault = false;
    uniqueid->QueryBoolAttribute("default", &isDefault);
    const std::string value = uniqueid->FirstChild() ? uniqueid->FirstChild()->ValueStr() : "";
    keys.emplace_back(type + ":" + value + ":" + (isDefault ? "default" : ""));
    uniqueid = uniqueid->NextSiblingElement("uniqueid");
  }

  return keys;
}

std::vector<std::string> GetSavedRatingKeys(const CXBMCTinyXML& doc)
{
  std::vector<std::string> keys;

  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return keys;

  const TiXmlElement* ratings = movie->FirstChildElement("ratings");
  if (!ratings)
    return keys;

  const TiXmlElement* rating = ratings->FirstChildElement("rating");
  while (rating)
  {
    std::string name;
    rating->QueryStringAttribute("name", &name);
    bool isDefault = false;
    rating->QueryBoolAttribute("default", &isDefault);
    keys.emplace_back(name + ":" + (isDefault ? "default" : ""));
    rating = rating->NextSiblingElement("rating");
  }

  return keys;
}

SActorInfo MakeActor(std::string name, std::string role, int order)
{
  SActorInfo actor;
  actor.strName = std::move(name);
  actor.strRole = std::move(role);
  actor.order = order;
  return actor;
}
} // namespace

TEST(TestVideoInfoTag, ReadTVShowSeasons)
{
  const std::string document =
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
         <tvshow>
         <namedseason number="1">season 1</namedseason>
         <namedseason number="2"></namedseason>
         <namedseason number="3"></namedseason>
         <namedseason number="4">season 4</namedseason>
         <seasonplot number="3">plot 3</seasonplot>
         <seasonplot number="4">plot 4</seasonplot>
         <seasonplot number="5"></seasonplot>
         </tvshow>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  const std::map<int, CVideoInfoTag::SeasonAttributes> reference = {
      {1, {"season 1", ""}}, {3, {"", "plot 3"}}, {4, {"season 4", "plot 4"}}};

  EXPECT_EQ(details.m_seasons, reference);
}

TEST(TestVideoInfoTag, SaveAfterLoadNativeWritesDeterministicTagOrder)
{
  const std::string document =
      R"(<movie>
           <tag>secret agent</tag>
           <tag>undercover</tag>
           <tag>killing</tag>
           <tag>british secret service</tag>
           <tag>mi6</tag>
         </movie>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  CXBMCTinyXML savedDoc;
  EXPECT_TRUE(details.Save(&savedDoc, "movie"));

  const std::vector<std::string> reference = {"british secret service", "killing", "mi6",
                                              "secret agent", "undercover"};
  EXPECT_EQ(GetSavedTags(savedDoc), reference);
}

TEST(TestVideoInfoTag, SaveWritesDeterministicRepeatedScalarOrder)
{
  CVideoInfoTag details;
  details.SetGenre({"thriller", "Action", "adventure"});
  details.SetCountry({"United States", "Canada", "Australia"});
  details.SetTags({"secret agent", "undercover", "british secret service"});
  details.SetWritingCredits({"Zed Writer", "amy Writer", "Amy Writer"});
  details.SetDirector({"Zed Director", "amy Director"});
  details.SetStudio({"Studio Z", "studio a"});

  CXBMCTinyXML savedDoc;
  EXPECT_TRUE(details.Save(&savedDoc, "movie"));

  EXPECT_EQ(GetSavedStrings(savedDoc, "genre"),
            (std::vector<std::string>{"Action", "adventure", "thriller"}));
  EXPECT_EQ(GetSavedStrings(savedDoc, "country"),
            (std::vector<std::string>{"Australia", "Canada", "United States"}));
  EXPECT_EQ(GetSavedTags(savedDoc),
            (std::vector<std::string>{"british secret service", "secret agent", "undercover"}));
  EXPECT_EQ(GetSavedStrings(savedDoc, "credits"),
            (std::vector<std::string>{"Amy Writer", "amy Writer", "Zed Writer"}));
  EXPECT_EQ(GetSavedStrings(savedDoc, "director"),
            (std::vector<std::string>{"amy Director", "Zed Director"}));
  EXPECT_EQ(GetSavedStrings(savedDoc, "studio"),
            (std::vector<std::string>{"studio a", "Studio Z"}));
}

TEST(TestVideoInfoTag, SaveWritesDeterministicCastOrder)
{
  CVideoInfoTag details;
  SActorInfo thumbTieB = MakeActor("Tie Actor", "Same Role", 3);
  thumbTieB.thumbUrl.AddParsedUrl("https://image.tmdb.org/t/p/original/cast-b.jpg", "thumb");
  SActorInfo thumbTieA = MakeActor("Tie Actor", "Same Role", 3);
  thumbTieA.thumbUrl.AddParsedUrl("https://image.tmdb.org/t/p/original/cast-a.jpg", "thumb");

  details.m_cast = {MakeActor("No Order", "Mystery", -1),
                    MakeActor("Zed Actor", "Second billed", 2),
                    MakeActor("Beta Actor", "Lead", 1),
                    MakeActor("Alpha Actor", "Lead", 1),
                    thumbTieB,
                    thumbTieA};

  CXBMCTinyXML savedDoc;
  EXPECT_TRUE(details.Save(&savedDoc, "movie"));

  EXPECT_EQ(GetSavedActors(savedDoc),
            (std::vector<std::string>{"Alpha Actor:Lead", "Beta Actor:Lead",
                                      "Zed Actor:Second billed", "Tie Actor:Same Role",
                                      "Tie Actor:Same Role", "No Order:Mystery"}));
  EXPECT_EQ(GetSavedActorKeys(savedDoc),
            (std::vector<std::string>{
                "Alpha Actor:Lead:",
                "Beta Actor:Lead:",
                "Zed Actor:Second billed:",
                "Tie Actor:Same Role:https://image.tmdb.org/t/p/original/cast-a.jpg",
                "Tie Actor:Same Role:https://image.tmdb.org/t/p/original/cast-b.jpg",
                "No Order:Mystery:"}));
}

TEST(TestVideoInfoTag, SaveWritesDeterministicProviderMapOrder)
{
  CVideoInfoTag details;
  details.SetUniqueID("64043", "tmdb", true);
  details.SetUniqueID("tt4577466", "imdb");
  details.SetUniqueID("299350", "tvdb");
  details.SetRating(CRating(8.5f, 10), "themoviedb", true);
  details.SetRating(CRating(7.5f, 20), "imdb");
  details.SetRating(CRating(6.5f, 30), "trakt");

  CXBMCTinyXML savedDoc;
  EXPECT_TRUE(details.Save(&savedDoc, "movie"));

  EXPECT_EQ(GetSavedUniqueIDKeys(savedDoc),
            (std::vector<std::string>{"imdb:tt4577466:", "tmdb:64043:default",
                                      "tvdb:299350:"}));
  EXPECT_EQ(GetSavedRatingKeys(savedDoc),
            (std::vector<std::string>{"imdb:", "themoviedb:default", "trakt:"}));
}

TEST(TestVideoInfoTag, SaveWritesDeterministicArtworkOrder)
{
  CVideoInfoTag details;
  details.m_strPictureURL.AddParsedUrl("https://image.tmdb.org/t/p/original/season9poster.jpg",
                                       "poster",
                                       "https://image.tmdb.org/t/p/w500/season9poster.jpg", "", "",
                                       false, false, 9);
  details.m_strPictureURL.AddParsedUrl("https://assets.fanart.tv/fanart/movieposter-b.jpg",
                                       "poster", "https://assets.fanart.tv/preview/movieposter-b.jpg",
                                       "", "", false, false);
  details.m_strPictureURL.AddParsedUrl("https://image.tmdb.org/t/p/original/backdrop-a.jpg",
                                       "fanart",
                                       "https://image.tmdb.org/t/p/w500/backdrop-a.jpg", "", "",
                                       false, false);
  details.m_strPictureURL.AddParsedUrl("https://image.tmdb.org/t/p/original/season2poster.jpg",
                                       "poster",
                                       "https://image.tmdb.org/t/p/w500/season2poster.jpg", "", "",
                                       false, false, 2);
  details.m_strPictureURL.AddParsedUrl("https://assets.fanart.tv/fanart/movieposter-a.jpg",
                                       "poster", "https://assets.fanart.tv/preview/movieposter-a.jpg",
                                       "", "", false, false);

  details.m_fanart.AddFanart("https://image.tmdb.org/t/p/original/fanart-c.jpg",
                             "https://image.tmdb.org/t/p/w500/fanart-c.jpg", "");
  details.m_fanart.AddFanart("https://assets.fanart.tv/fanart/background-a.jpg",
                             "https://assets.fanart.tv/preview/background-a.jpg", "");
  details.m_fanart.AddFanart("https://image.tmdb.org/t/p/original/fanart-a.jpg",
                             "https://image.tmdb.org/t/p/w500/fanart-a.jpg", "");
  details.m_fanart.Pack();

  CXBMCTinyXML savedDoc;
  EXPECT_TRUE(details.Save(&savedDoc, "movie"));

  EXPECT_EQ(GetSavedThumbKeys(savedDoc),
            (std::vector<std::string>{
                "::poster::::https://assets.fanart.tv/fanart/movieposter-a.jpg:"
                "https://assets.fanart.tv/preview/movieposter-a.jpg",
                "::poster::::https://assets.fanart.tv/fanart/movieposter-b.jpg:"
                "https://assets.fanart.tv/preview/movieposter-b.jpg",
                "season:2:poster::::https://image.tmdb.org/t/p/original/season2poster.jpg:"
                "https://image.tmdb.org/t/p/w500/season2poster.jpg",
                "season:9:poster::::https://image.tmdb.org/t/p/original/season9poster.jpg:"
                "https://image.tmdb.org/t/p/w500/season9poster.jpg",
                "::fanart::::https://image.tmdb.org/t/p/original/backdrop-a.jpg:"
                "https://image.tmdb.org/t/p/w500/backdrop-a.jpg"}));

  EXPECT_EQ(GetSavedFanartThumbKeys(savedDoc),
            (std::vector<std::string>{
                "https://assets.fanart.tv/fanart/background-a.jpg:"
                "https://assets.fanart.tv/preview/background-a.jpg:",
                "https://image.tmdb.org/t/p/original/fanart-a.jpg:"
                "https://image.tmdb.org/t/p/w500/fanart-a.jpg:",
                "https://image.tmdb.org/t/p/original/fanart-c.jpg:"
                "https://image.tmdb.org/t/p/w500/fanart-c.jpg:"}));
}

TEST(TestVideoInfoTag, SavePreservesFanartRootAttributesWhenSorting)
{
  CVideoInfoTag details;
  details.m_fanart.m_xml =
      R"(<fanart url="https://assets.fanart.tv/fanart/">)"
      R"(<thumb preview="preview/background-b.jpg">background-b.jpg</thumb>)"
      R"(<thumb preview="preview/background-a.jpg">background-a.jpg</thumb>)"
      R"(</fanart>)";

  CXBMCTinyXML savedDoc;
  EXPECT_TRUE(details.Save(&savedDoc, "movie"));

  EXPECT_EQ(GetSavedFanartAttribute(savedDoc, "url"), "https://assets.fanart.tv/fanart/");
  EXPECT_EQ(GetSavedFanartThumbKeys(savedDoc),
            (std::vector<std::string>{"background-a.jpg:preview/background-a.jpg:",
                                      "background-b.jpg:preview/background-b.jpg:"}));
}

TEST(TestVideoInfoTag, SaveSkipsMalformedFanartXml)
{
  CVideoInfoTag details;
  details.m_fanart.m_xml = R"(<thumb preview="preview/background-a.jpg">background-a.jpg</thumb>)";

  CXBMCTinyXML savedDoc;
  EXPECT_TRUE(details.Save(&savedDoc, "movie"));

  EXPECT_FALSE(HasSavedFanart(savedDoc));

  details.m_fanart.m_xml = R"(<fanart><thumb preview="preview/background-a.jpg">background-a.jpg)";

  CXBMCTinyXML malformedDoc;
  EXPECT_TRUE(details.Save(&malformedDoc, "movie"));

  EXPECT_FALSE(HasSavedFanart(malformedDoc));
}

TEST(TestVideoInfoTag, SaveSkipsTopLevelArtworkXmlWithoutThumbRoot)
{
  CVideoInfoTag details;
  details.m_strPictureURL.SetData(R"(<poster>https://image.tmdb.org/t/p/original/poster.jpg</poster>)");

  CXBMCTinyXML savedDoc;
  EXPECT_TRUE(details.Save(&savedDoc, "movie"));

  EXPECT_FALSE(HasSavedThumb(savedDoc));
}

// Trick to make protected methods accessible for testing
class CVideoInfoTagTest : public CVideoInfoTag
{
public:
  bool ForwardSaveTvShowSeasons(TiXmlNode* node) { return SaveTvShowSeasons(node); }
};

TEST(TestVideoInfoTag, SaveTVShowSeasons)
{
  const std::map<int, CVideoInfoTag::SeasonAttributes> reference = {
      {1, {"season 1", "plot 1"}}, {2, {"", "plot 2"}}, {3, {"season 3", ""}}, {4, {"", ""}}};

  const std::string referenceXml = R"(<namedseason number="1">season 1</namedseason>
<seasonplot number="1">plot 1</seasonplot>
<seasonplot number="2">plot 2</seasonplot>
<namedseason number="3">season 3</namedseason>
)";

  CVideoInfoTagTest details;
  details.SetSeasons(reference);

  CXBMCTinyXML xmlDoc;
  details.ForwardSaveTvShowSeasons(&xmlDoc);

  TiXmlPrinter printer;
  xmlDoc.Accept(&printer);
  std::string result = printer.Str();

  EXPECT_EQ(result, referenceXml);
}

TEST(TestVideoInfoTag, SetUniqueIDs)
{
  // initial state: no default, empty list.
  CVideoInfoTag details;
  std::map<std::string, std::string, std::less<>> reference = {};

  EXPECT_EQ(details.GetDefaultUniqueID(), "unknown");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // usual flow: initialize from initial state with a list.
  // entries with blank type or uniqueid are ignored
  std::map<std::string, std::string, std::less<>> test = {
      {"imdb", "tt4577466"}, {"tmdb", "64043"}, {"tvdb", "299350"}, {"", "123456"}, {"foo", ""}};
  reference = {{"imdb", "tt4577466"}, {"tmdb", "64043"}, {"tvdb", "299350"}};

  details.SetUniqueIDs(test);
  details.SetUniqueID("64043", "tmdb", true);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // current update behavior, not sure why:
  // the former default type and value from the previous list of uniqueids are added back when
  // omitted from the new list - instead of reverting to "unknown" default and setting the list as provided.
  test = {{"imdb", "tt4577466"}, {"tvdb", "299350"}};
  details.SetUniqueIDs(test);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // setting a blank list clears all except the previous default
  test = {};
  reference = {{"tmdb", "64043"}};
  details.SetUniqueIDs(test);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // except when there is no explicit default, then setting a blank list clears the list.
  CVideoInfoTag details2;
  details2.SetUniqueIDs(reference);
  details2.SetUniqueIDs(test);

  EXPECT_EQ(details2.GetDefaultUniqueID(), "unknown");
  EXPECT_EQ(details2.GetUniqueIDs(), test);
}

struct TestOriginalLanguage
{
  std::string input;
  std::string expected;
  CVideoInfoTag::LanguageTagSource source = CVideoInfoTag::LanguageTagSource::SOURCE_EXTERNAL;
  bool status = true;
};

std::ostream& operator<<(std::ostream& os, const TestOriginalLanguage& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestOriginalLanguage OriginalLanguageTests[] = {
    {"en", "en", CVideoInfoTag::LanguageTagSource::SOURCE_INTERNAL},
    {"foobarbaz", "foobarbaz", CVideoInfoTag::LanguageTagSource::SOURCE_INTERNAL},
    {"en", "en"}, // ISO 639-1
    {"eng", "en"}, // ISO 639-2
    {"fra", "fr"}, // ISO 639-2/T
    {"fre", "fr"}, // ISO 639-2/B
    {"en-US", "en-US"}, // BCP 47 lang-region
    {"zh-guoyu", "zh-guoyu"}, // Grandfathered BCP 47
    // Future: expected to be rewritten to the preferred language defined in the registry
    // Other tests for canonicalization will be needed as well
    {"english", "en"}, // English name
    {"foobarbaz", "", CVideoInfoTag::LanguageTagSource::SOURCE_EXTERNAL, false}, // Unknown English name
};
// clang-format on

class OriginalLanguageTester : public testing::Test,
                               public testing::WithParamInterface<TestOriginalLanguage>
{
};

TEST_P(OriginalLanguageTester, SetOriginalLanguage)
{
  auto& param = GetParam();

  CVideoInfoTag tag;
  bool status = tag.SetOriginalLanguage(param.input, param.source);
  EXPECT_EQ(param.status, status);
  if (status)
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.expected, tag.GetOriginalLanguage());
  }
}

INSTANTIATE_TEST_SUITE_P(TestVideoInfoTag,
                         OriginalLanguageTester,
                         testing::ValuesIn(OriginalLanguageTests));
