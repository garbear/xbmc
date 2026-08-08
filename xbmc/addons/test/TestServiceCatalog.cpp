/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/ServiceCatalog.h"
#include "filesystem/File.h"
#include "filesystem/PipesManager.h"
#include "test/TestUtils.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

using namespace ADDON;

namespace
{
using Error = CServiceCatalog::Error;

constexpr std::string_view MINIMAL_CATALOG = R"({"version":1,"items":[]})";
constexpr std::string_view SINGLE_ITEM_CATALOG =
    R"({"version":1,"items":[{"id":"cinematic.earth.example","name":"Example","media":"https://example.test/video.m3u8"}]})";
constexpr std::string_view DATA_URI =
    "data:application/json;base64,"
    "eyJ2ZXJzaW9uIjoxLCJpdGVtcyI6W3siaWQiOiJleGFtcGxlIiwibmFtZSI6IkV4YW1wbGUi"
    "LCJtZWRpYSI6ImRhdGE6dmlkZW8vbXAydCxhYmMifV19";

struct TempFileDeleter
{
  void operator()(XFILE::CFile* file) const { XBMC_DELETETEMPFILE(file); }
};

using TempFilePtr = std::unique_ptr<XFILE::CFile, TempFileDeleter>;

struct PipeDeleter
{
  void operator()(XFILE::Pipe* pipe) const { XFILE::PipesManager::GetInstance().ClosePipe(pipe); }
};

using PipePtr = std::unique_ptr<XFILE::Pipe, PipeDeleter>;

void ExpectParseFailure(std::string_view json, Error expectedError)
{
  SCOPED_TRACE(std::string{json});

  CServiceCatalog catalog;
  Error error{Error::NONE};
  EXPECT_FALSE(CServiceCatalog::Parse(std::string{json}, catalog, &error));
  EXPECT_EQ(expectedError, error);
}

std::string CatalogWithSize(std::size_t size)
{
  constexpr std::string_view prefix = R"({"version":1,"items":[],"padding":")";
  constexpr std::string_view suffix = R"("})";

  EXPECT_GE(size, prefix.size() + suffix.size());

  std::string catalog{prefix};
  catalog.append(size - prefix.size() - suffix.size(), 'x');
  catalog.append(suffix);
  return catalog;
}

PipePtr CreatePipeWithContents(std::string_view contents)
{
  PipePtr pipe{XFILE::PipesManager::GetInstance().CreatePipe()};
  if (pipe == nullptr || !pipe->Write(contents.data(), static_cast<int>(contents.size())))
    return nullptr;

  pipe->SetEof();
  return pipe;
}
} // namespace

TEST(TestServiceCatalog, ParseMinimalEmptyCatalog)
{
  CServiceCatalog catalog;
  Error error{Error::UNKNOWN};

  ASSERT_TRUE(CServiceCatalog::Parse(std::string{MINIMAL_CATALOG}, catalog, &error));
  EXPECT_EQ(Error::NONE, error);
  EXPECT_EQ(1U, catalog.Version());
  EXPECT_TRUE(catalog.Items().empty());
}

TEST(TestServiceCatalog, ParseSingleItem)
{
  CServiceCatalog catalog;

  ASSERT_TRUE(CServiceCatalog::Parse(std::string{SINGLE_ITEM_CATALOG}, catalog));
  ASSERT_EQ(1U, catalog.Items().size());
  EXPECT_EQ("cinematic.earth.example", catalog.Items()[0].id);
  EXPECT_EQ("Example", catalog.Items()[0].name);
  EXPECT_EQ("https://example.test/video.m3u8", catalog.Items()[0].media);
}

TEST(TestServiceCatalog, ParsePreservesItemOrderAndDuplicateIds)
{
  constexpr std::string_view json =
      R"({"version":1,"items":[{"id":"duplicate","name":"First","media":"first:"},{"id":"second","name":"Second","media":"second:"},{"id":"duplicate","name":"Third","media":"third:"}]})";
  CServiceCatalog catalog;

  ASSERT_TRUE(CServiceCatalog::Parse(std::string{json}, catalog));
  ASSERT_EQ(3U, catalog.Items().size());
  EXPECT_EQ("First", catalog.Items()[0].name);
  EXPECT_EQ("Second", catalog.Items()[1].name);
  EXPECT_EQ("Third", catalog.Items()[2].name);
  EXPECT_EQ("duplicate", catalog.Items()[0].id);
  EXPECT_EQ("duplicate", catalog.Items()[2].id);
}

TEST(TestServiceCatalog, ParseIgnoresUnknownFields)
{
  constexpr std::string_view json =
      R"({"version":1,"futureTopLevel":{"whatever":true},"items":[{"id":"example","name":"Example","media":"https://example.test/video.m3u8","futureItemField":[1,2,3]}]})";
  CServiceCatalog catalog;

  ASSERT_TRUE(CServiceCatalog::Parse(std::string{json}, catalog));
  ASSERT_EQ(1U, catalog.Items().size());
  EXPECT_EQ("example", catalog.Items()[0].id);
}

TEST(TestServiceCatalog, ParsePreservesStringContents)
{
  constexpr std::string_view json =
      R"({"version":1,"items":[{"id":"  Case.Sensitive / ID  ","name":"  Service Name / v1  ","media":"  data:application/vnd.apple.mpegurl,%23EXTM3U  "}]})";
  CServiceCatalog catalog;

  ASSERT_TRUE(CServiceCatalog::Parse(std::string{json}, catalog));
  ASSERT_EQ(1U, catalog.Items().size());
  EXPECT_EQ("  Case.Sensitive / ID  ", catalog.Items()[0].id);
  EXPECT_EQ("  Service Name / v1  ", catalog.Items()[0].name);
  EXPECT_EQ("  data:application/vnd.apple.mpegurl,%23EXTM3U  ", catalog.Items()[0].media);
}

TEST(TestServiceCatalog, ParseRejectsMalformedJson)
{
  ExpectParseFailure(R"({"version":1)", Error::MALFORMED_JSON);

  std::string jsonWithRawNull{MINIMAL_CATALOG};
  jsonWithRawNull.push_back('\0');
  jsonWithRawNull.append("{}");
  ExpectParseFailure(jsonWithRawNull, Error::MALFORMED_JSON);
}

TEST(TestServiceCatalog, ParseRejectsNonObjectRoots)
{
  ExpectParseFailure("[]", Error::ROOT_NOT_OBJECT);
  ExpectParseFailure(R"("foo")", Error::ROOT_NOT_OBJECT);
  ExpectParseFailure("123", Error::ROOT_NOT_OBJECT);
  ExpectParseFailure("null", Error::ROOT_NOT_OBJECT);
}

TEST(TestServiceCatalog, ParseRejectsMissingTopLevelFields)
{
  ExpectParseFailure("{}", Error::MISSING_VERSION);
  ExpectParseFailure(R"({"items":[]})", Error::MISSING_VERSION);
  ExpectParseFailure(R"({"version":1})", Error::MISSING_ITEMS);
}

TEST(TestServiceCatalog, ParseRejectsWrongTopLevelFieldTypes)
{
  ExpectParseFailure(R"({"version":"1","items":[]})", Error::INVALID_VERSION_TYPE);
  ExpectParseFailure(R"({"version":1.0,"items":[]})", Error::INVALID_VERSION_TYPE);
  ExpectParseFailure(R"({"version":null,"items":[]})", Error::INVALID_VERSION_TYPE);
  ExpectParseFailure(R"({"version":1,"items":null})", Error::INVALID_ITEMS_TYPE);
  ExpectParseFailure(R"({"version":1,"items":{}})", Error::INVALID_ITEMS_TYPE);
  ExpectParseFailure(R"({"version":1,"items":"items"})", Error::INVALID_ITEMS_TYPE);
}

TEST(TestServiceCatalog, ParseRejectsInvalidItemElementTypes)
{
  ExpectParseFailure(R"({"version":1,"items":[null]})", Error::INVALID_ITEM_TYPE);
  ExpectParseFailure(R"({"version":1,"items":["foo"]})", Error::INVALID_ITEM_TYPE);
  ExpectParseFailure(R"({"version":1,"items":[123]})", Error::INVALID_ITEM_TYPE);
}

TEST(TestServiceCatalog, ParseRejectsMissingItemFields)
{
  ExpectParseFailure(R"({"version":1,"items":[{"name":"Name","media":"media:"}]})",
                     Error::MISSING_ITEM_ID);
  ExpectParseFailure(R"({"version":1,"items":[{"id":"id","media":"media:"}]})",
                     Error::MISSING_ITEM_NAME);
  ExpectParseFailure(R"({"version":1,"items":[{"id":"id","name":"Name"}]})",
                     Error::MISSING_ITEM_MEDIA);
}

TEST(TestServiceCatalog, ParseRejectsWrongItemFieldTypes)
{
  ExpectParseFailure(R"({"version":1,"items":[{"id":null,"name":"Name","media":"media:"}]})",
                     Error::INVALID_ITEM_ID_TYPE);
  ExpectParseFailure(R"({"version":1,"items":[{"id":"id","name":123,"media":"media:"}]})",
                     Error::INVALID_ITEM_NAME_TYPE);
  ExpectParseFailure(R"({"version":1,"items":[{"id":"id","name":"Name","media":{}}]})",
                     Error::INVALID_ITEM_MEDIA_TYPE);
}

TEST(TestServiceCatalog, ParseRejectsEmptyItemStrings)
{
  ExpectParseFailure(R"({"version":1,"items":[{"id":"","name":"Name","media":"media:"}]})",
                     Error::EMPTY_ITEM_ID);
  ExpectParseFailure(R"({"version":1,"items":[{"id":"id","name":"","media":"media:"}]})",
                     Error::EMPTY_ITEM_NAME);
  ExpectParseFailure(R"({"version":1,"items":[{"id":"id","name":"Name","media":""}]})",
                     Error::EMPTY_ITEM_MEDIA);
}

TEST(TestServiceCatalog, ParseRejectsUnsupportedVersions)
{
  ExpectParseFailure(R"({"version":0,"items":[]})", Error::UNSUPPORTED_VERSION);
  ExpectParseFailure(R"({"version":2,"items":[]})", Error::UNSUPPORTED_VERSION);
  ExpectParseFailure(R"({"version":-1,"items":[]})", Error::UNSUPPORTED_VERSION);
  ExpectParseFailure(R"({"version":4294967296,"items":[]})", Error::UNSUPPORTED_VERSION);
}

TEST(TestServiceCatalog, ParseFailureDoesNotModifyCatalog)
{
  constexpr std::string_view existingJson =
      R"({"version":1,"items":[{"id":"existing-one","name":"Existing One","media":"one:"},{"id":"existing-two","name":"Existing Two","media":"two:"}]})";
  constexpr std::string_view invalidReplacement =
      R"({"version":1,"items":[{"id":"replacement-one","name":"Replacement One","media":"replacement-one:"},{"id":"replacement-two","name":"Replacement Two","media":""}]})";
  CServiceCatalog catalog;
  ASSERT_TRUE(CServiceCatalog::Parse(std::string{existingJson}, catalog));

  Error error{Error::NONE};
  EXPECT_FALSE(CServiceCatalog::Parse(std::string{invalidReplacement}, catalog, &error));
  EXPECT_EQ(Error::EMPTY_ITEM_MEDIA, error);
  EXPECT_EQ(1U, catalog.Version());
  ASSERT_EQ(2U, catalog.Items().size());
  EXPECT_EQ("existing-one", catalog.Items()[0].id);
  EXPECT_EQ("Existing One", catalog.Items()[0].name);
  EXPECT_EQ("one:", catalog.Items()[0].media);
  EXPECT_EQ("existing-two", catalog.Items()[1].id);
  EXPECT_EQ("Existing Two", catalog.Items()[1].name);
  EXPECT_EQ("two:", catalog.Items()[1].media);
}

TEST(TestServiceCatalog, LoadCanonicalDataUri)
{
  CServiceCatalog catalog;
  Error error{Error::UNKNOWN};

  ASSERT_TRUE(CServiceCatalog::Load(std::string{DATA_URI}, catalog, &error));
  EXPECT_EQ(Error::NONE, error);
  EXPECT_EQ(1U, catalog.Version());
  ASSERT_EQ(1U, catalog.Items().size());
  EXPECT_EQ("example", catalog.Items()[0].id);
  EXPECT_EQ("Example", catalog.Items()[0].name);
  EXPECT_EQ("data:video/mp2t,abc", catalog.Items()[0].media);
}

TEST(TestServiceCatalog, LoadLocalResource)
{
  TempFilePtr file{XBMC_CREATETEMPFILE(".json")};
  ASSERT_NE(nullptr, file);
  ASSERT_EQ(static_cast<ssize_t>(SINGLE_ITEM_CATALOG.size()),
            file->Write(SINGLE_ITEM_CATALOG.data(), SINGLE_ITEM_CATALOG.size()));
  const std::string path = XBMC_TEMPFILEPATH(file.get());
  file->Close();

  CServiceCatalog catalog;
  Error error{Error::UNKNOWN};
  ASSERT_TRUE(CServiceCatalog::Load(path, catalog, &error));
  EXPECT_EQ(Error::NONE, error);
  ASSERT_EQ(1U, catalog.Items().size());
  EXPECT_EQ("cinematic.earth.example", catalog.Items()[0].id);
}

TEST(TestServiceCatalog, LoadRejectsMissingResource)
{
  TempFilePtr missingFile{XBMC_CREATETEMPFILE(".json")};
  ASSERT_NE(nullptr, missingFile);
  const std::string path = XBMC_TEMPFILEPATH(missingFile.get());
  missingFile.reset();

  CServiceCatalog catalog;
  Error error{Error::NONE};
  EXPECT_FALSE(CServiceCatalog::Load(path, catalog, &error));
  EXPECT_EQ(Error::OPEN_FAILED, error);
}

TEST(TestServiceCatalog, LoadRejectsMalformedResourceWithoutModifyingCatalog)
{
  constexpr std::string_view invalidJson{R"({"version":1)"};
  TempFilePtr file{XBMC_CREATETEMPFILE(".json")};
  ASSERT_NE(nullptr, file);
  ASSERT_EQ(static_cast<ssize_t>(invalidJson.size()),
            file->Write(invalidJson.data(), invalidJson.size()));
  const std::string path = XBMC_TEMPFILEPATH(file.get());
  file->Close();

  CServiceCatalog catalog;
  ASSERT_TRUE(CServiceCatalog::Parse(std::string{SINGLE_ITEM_CATALOG}, catalog));

  Error error{Error::NONE};
  EXPECT_FALSE(CServiceCatalog::Load(path, catalog, &error));
  EXPECT_EQ(Error::MALFORMED_JSON, error);
  EXPECT_EQ(1U, catalog.Version());
  ASSERT_EQ(1U, catalog.Items().size());
  EXPECT_EQ("cinematic.earth.example", catalog.Items()[0].id);
}

TEST(TestServiceCatalog, LoadAcceptsResourceAtSizeLimit)
{
  const std::string json = CatalogWithSize(CServiceCatalog::MAX_RESOURCE_SIZE);
  ASSERT_EQ(CServiceCatalog::MAX_RESOURCE_SIZE, json.size());
  PipePtr pipe = CreatePipeWithContents(json);
  ASSERT_NE(nullptr, pipe);

  CServiceCatalog catalog;
  Error error{Error::UNKNOWN};
  ASSERT_TRUE(CServiceCatalog::Load(pipe->GetName(), catalog, &error));
  EXPECT_EQ(Error::NONE, error);
  EXPECT_EQ(1U, catalog.Version());
  EXPECT_TRUE(catalog.Items().empty());
}

TEST(TestServiceCatalog, LoadRejectsResourceOverSizeLimit)
{
  const std::string json = CatalogWithSize(CServiceCatalog::MAX_RESOURCE_SIZE + 1);
  ASSERT_EQ(CServiceCatalog::MAX_RESOURCE_SIZE + 1, json.size());
  PipePtr pipe = CreatePipeWithContents(json);
  ASSERT_NE(nullptr, pipe);

  CServiceCatalog catalog;
  Error error{Error::NONE};
  EXPECT_FALSE(CServiceCatalog::Load(pipe->GetName(), catalog, &error));
  EXPECT_EQ(Error::RESOURCE_TOO_LARGE, error);
}

TEST(TestServiceCatalog, LoadReportsReadFailure)
{
  PipePtr pipe{XFILE::PipesManager::GetInstance().CreatePipe()};
  ASSERT_NE(nullptr, pipe);
  const std::string name = pipe->GetName();
  pipe->Close();

  CServiceCatalog catalog;
  Error error{Error::NONE};
  EXPECT_FALSE(CServiceCatalog::Load(name, catalog, &error));
  EXPECT_EQ(Error::READ_FAILED, error);
}
