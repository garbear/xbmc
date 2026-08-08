/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/ServiceManifest.h"
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
using Error = CServiceManifest::Error;

constexpr std::string_view MINIMAL_MANIFEST =
    R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth"})";
constexpr std::string_view DATA_URI =
    "data:application/json;base64,"
    "eyJ2ZXJzaW9uIjoxLCJpZCI6ImNpbmVtYXRpYy5lYXJ0aCIsIm5hbWUiOiJjaW5lbWF0aWMu"
    "ZWFydGgifQ==";

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

  CServiceManifest manifest;
  Error error{Error::NONE};
  EXPECT_FALSE(CServiceManifest::Parse(std::string{json}, manifest, &error));
  EXPECT_EQ(expectedError, error);
}

std::string ManifestWithSize(size_t size)
{
  constexpr std::string_view prefix =
      R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth","padding":")";
  constexpr std::string_view suffix = R"("})";

  EXPECT_GE(size, prefix.size() + suffix.size());

  std::string manifest{prefix};
  manifest.append(size - prefix.size() - suffix.size(), 'x');
  manifest.append(suffix);
  return manifest;
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

TEST(TestServiceManifest, ParseMinimalVersionOne)
{
  CServiceManifest manifest;
  Error error{Error::UNKNOWN};

  ASSERT_TRUE(CServiceManifest::Parse(std::string{MINIMAL_MANIFEST}, manifest, &error));
  EXPECT_EQ(Error::NONE, error);
  EXPECT_EQ(1U, manifest.Version());
  EXPECT_EQ("cinematic.earth", manifest.ID());
  EXPECT_EQ("cinematic.earth", manifest.Name());
  EXPECT_TRUE(manifest.Catalog().empty());
}

TEST(TestServiceManifest, ParseVersionOneWithCatalog)
{
  constexpr std::string_view json =
      R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth","catalog":"https://example.test/catalog.json"})";
  CServiceManifest manifest;

  ASSERT_TRUE(CServiceManifest::Parse(std::string{json}, manifest));
  EXPECT_EQ("https://example.test/catalog.json", manifest.Catalog());
}

TEST(TestServiceManifest, ParseIgnoresUnknownFields)
{
  constexpr std::string_view json =
      R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth","catalog":"https://example.test/catalog.json","unknown":{"anything":true}})";
  CServiceManifest manifest;

  ASSERT_TRUE(CServiceManifest::Parse(std::string{json}, manifest));
  EXPECT_EQ(1U, manifest.Version());
  EXPECT_EQ("cinematic.earth", manifest.ID());
  EXPECT_EQ("cinematic.earth", manifest.Name());
  EXPECT_EQ("https://example.test/catalog.json", manifest.Catalog());
}

TEST(TestServiceManifest, ParsePreservesStringContents)
{
  constexpr std::string_view json =
      R"({"version":1,"id":"  Case.Sensitive-ID  ","name":"  Service Name / v1  "})";
  CServiceManifest manifest;

  ASSERT_TRUE(CServiceManifest::Parse(std::string{json}, manifest));
  EXPECT_EQ("  Case.Sensitive-ID  ", manifest.ID());
  EXPECT_EQ("  Service Name / v1  ", manifest.Name());
}

TEST(TestServiceManifest, ParsePreservesCatalogContents)
{
  constexpr std::string_view json =
      R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth","catalog":"  data:application/json,%7B%7D  "})";
  CServiceManifest manifest;

  ASSERT_TRUE(CServiceManifest::Parse(std::string{json}, manifest));
  EXPECT_EQ("  data:application/json,%7B%7D  ", manifest.Catalog());
}

TEST(TestServiceManifest, ParseFailureDoesNotModifyManifest)
{
  CServiceManifest manifest;
  ASSERT_TRUE(CServiceManifest::Parse(std::string{MINIMAL_MANIFEST}, manifest));

  Error error{Error::NONE};
  EXPECT_FALSE(CServiceManifest::Parse("{}", manifest, &error));
  EXPECT_EQ(Error::MISSING_VERSION, error);
  EXPECT_EQ(1U, manifest.Version());
  EXPECT_EQ("cinematic.earth", manifest.ID());
  EXPECT_EQ("cinematic.earth", manifest.Name());
}

TEST(TestServiceManifest, InvalidCatalogDoesNotModifyManifest)
{
  constexpr std::string_view validJson =
      R"({"version":1,"id":"existing.id","name":"Existing Name","catalog":"data:application/json,%7B%7D"})";
  constexpr std::string_view invalidJson =
      R"({"version":1,"id":"replacement.id","name":"Replacement Name","catalog":null})";
  CServiceManifest manifest;
  ASSERT_TRUE(CServiceManifest::Parse(std::string{validJson}, manifest));

  Error error{Error::NONE};
  EXPECT_FALSE(CServiceManifest::Parse(std::string{invalidJson}, manifest, &error));
  EXPECT_EQ(Error::INVALID_CATALOG_TYPE, error);
  EXPECT_EQ(1U, manifest.Version());
  EXPECT_EQ("existing.id", manifest.ID());
  EXPECT_EQ("Existing Name", manifest.Name());
  EXPECT_EQ("data:application/json,%7B%7D", manifest.Catalog());
}

TEST(TestServiceManifest, ParseRejectsMalformedJson)
{
  ExpectParseFailure(R"({"version":1)", Error::MALFORMED_JSON);

  std::string jsonWithRawNull{MINIMAL_MANIFEST};
  jsonWithRawNull.push_back('\0');
  jsonWithRawNull.append("{}");
  ExpectParseFailure(jsonWithRawNull, Error::MALFORMED_JSON);
}

TEST(TestServiceManifest, ParseRejectsNonObjectRoots)
{
  ExpectParseFailure("[]", Error::ROOT_NOT_OBJECT);
  ExpectParseFailure(R"("foo")", Error::ROOT_NOT_OBJECT);
  ExpectParseFailure("123", Error::ROOT_NOT_OBJECT);
  ExpectParseFailure("null", Error::ROOT_NOT_OBJECT);
}

TEST(TestServiceManifest, ParseRejectsMissingFields)
{
  ExpectParseFailure("{}", Error::MISSING_VERSION);
  ExpectParseFailure(R"({"id":"cinematic.earth","name":"cinematic.earth"})",
                     Error::MISSING_VERSION);
  ExpectParseFailure(R"({"version":1,"name":"cinematic.earth"})", Error::MISSING_ID);
  ExpectParseFailure(R"({"version":1,"id":"cinematic.earth"})", Error::MISSING_NAME);
}

TEST(TestServiceManifest, ParseRejectsWrongFieldTypes)
{
  ExpectParseFailure(R"({"version":"1","id":"cinematic.earth","name":"cinematic.earth"})",
                     Error::INVALID_VERSION_TYPE);
  ExpectParseFailure(R"({"version":1.0,"id":"cinematic.earth","name":"cinematic.earth"})",
                     Error::INVALID_VERSION_TYPE);
  ExpectParseFailure(R"({"version":null,"id":"cinematic.earth","name":"cinematic.earth"})",
                     Error::INVALID_VERSION_TYPE);
  ExpectParseFailure(R"({"version":1,"id":123,"name":"cinematic.earth"})", Error::INVALID_ID_TYPE);
  ExpectParseFailure(R"({"version":1,"id":null,"name":"cinematic.earth"})", Error::INVALID_ID_TYPE);
  ExpectParseFailure(R"({"version":1,"id":"cinematic.earth","name":false})",
                     Error::INVALID_NAME_TYPE);
  ExpectParseFailure(R"({"version":1,"id":"cinematic.earth","name":null})",
                     Error::INVALID_NAME_TYPE);
  ExpectParseFailure(
      R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth","catalog":null})",
      Error::INVALID_CATALOG_TYPE);
  ExpectParseFailure(
      R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth","catalog":123})",
      Error::INVALID_CATALOG_TYPE);
  ExpectParseFailure(
      R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth","catalog":{}})",
      Error::INVALID_CATALOG_TYPE);
  ExpectParseFailure(
      R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth","catalog":[]})",
      Error::INVALID_CATALOG_TYPE);
}

TEST(TestServiceManifest, ParseRejectsEmptyStrings)
{
  ExpectParseFailure(R"({"version":1,"id":"","name":"cinematic.earth"})", Error::EMPTY_ID);
  ExpectParseFailure(R"({"version":1,"id":"cinematic.earth","name":""})", Error::EMPTY_NAME);
  ExpectParseFailure(
      R"({"version":1,"id":"cinematic.earth","name":"cinematic.earth","catalog":""})",
      Error::EMPTY_CATALOG);
}

TEST(TestServiceManifest, ParseRejectsUnsupportedVersions)
{
  ExpectParseFailure(R"({"version":0,"id":"cinematic.earth","name":"cinematic.earth"})",
                     Error::UNSUPPORTED_VERSION);
  ExpectParseFailure(R"({"version":2,"id":"cinematic.earth","name":"cinematic.earth"})",
                     Error::UNSUPPORTED_VERSION);
  ExpectParseFailure(R"({"version":-1,"id":"cinematic.earth","name":"cinematic.earth"})",
                     Error::UNSUPPORTED_VERSION);
}

TEST(TestServiceManifest, LoadCanonicalDataUri)
{
  CServiceManifest manifest;
  Error error{Error::UNKNOWN};

  ASSERT_TRUE(CServiceManifest::Load(std::string{DATA_URI}, manifest, &error));
  EXPECT_EQ(Error::NONE, error);
  EXPECT_EQ(1U, manifest.Version());
  EXPECT_EQ("cinematic.earth", manifest.ID());
  EXPECT_EQ("cinematic.earth", manifest.Name());
}

TEST(TestServiceManifest, LoadRejectsMissingResource)
{
  TempFilePtr missingFile{XBMC_CREATETEMPFILE(".json")};
  ASSERT_NE(nullptr, missingFile);
  const std::string path = XBMC_TEMPFILEPATH(missingFile.get());
  missingFile.reset();

  CServiceManifest manifest;
  Error error{Error::NONE};

  EXPECT_FALSE(CServiceManifest::Load(path, manifest, &error));
  EXPECT_EQ(Error::OPEN_FAILED, error);
}

TEST(TestServiceManifest, LoadRejectsMalformedLocalResource)
{
  constexpr std::string_view invalidJson{R"({"version":1)"};
  TempFilePtr file{XBMC_CREATETEMPFILE(".json")};
  ASSERT_NE(nullptr, file);
  ASSERT_EQ(static_cast<ssize_t>(invalidJson.size()),
            file->Write(invalidJson.data(), invalidJson.size()));
  const std::string path = XBMC_TEMPFILEPATH(file.get());
  file->Close();

  CServiceManifest manifest;
  Error error{Error::NONE};
  EXPECT_FALSE(CServiceManifest::Load(path, manifest, &error));
  EXPECT_EQ(Error::MALFORMED_JSON, error);
}

TEST(TestServiceManifest, LoadAcceptsResourceAtSizeLimit)
{
  const std::string json = ManifestWithSize(CServiceManifest::MAX_RESOURCE_SIZE);
  ASSERT_EQ(CServiceManifest::MAX_RESOURCE_SIZE, json.size());
  PipePtr pipe = CreatePipeWithContents(json);
  ASSERT_NE(nullptr, pipe);

  CServiceManifest manifest;
  Error error{Error::UNKNOWN};
  ASSERT_TRUE(CServiceManifest::Load(pipe->GetName(), manifest, &error));
  EXPECT_EQ(Error::NONE, error);
  EXPECT_EQ(1U, manifest.Version());
  EXPECT_EQ("cinematic.earth", manifest.ID());
  EXPECT_EQ("cinematic.earth", manifest.Name());
}

TEST(TestServiceManifest, LoadRejectsResourceOverSizeLimit)
{
  const std::string json = ManifestWithSize(CServiceManifest::MAX_RESOURCE_SIZE + 1);
  ASSERT_EQ(CServiceManifest::MAX_RESOURCE_SIZE + 1, json.size());
  PipePtr pipe = CreatePipeWithContents(json);
  ASSERT_NE(nullptr, pipe);

  CServiceManifest manifest;
  Error error{Error::NONE};
  EXPECT_FALSE(CServiceManifest::Load(pipe->GetName(), manifest, &error));
  EXPECT_EQ(Error::RESOURCE_TOO_LARGE, error);
}

TEST(TestServiceManifest, LoadReportsReadFailure)
{
  PipePtr pipe{XFILE::PipesManager::GetInstance().CreatePipe()};
  ASSERT_NE(nullptr, pipe);
  const std::string name = pipe->GetName();
  pipe->Close();

  CServiceManifest manifest;
  Error error{Error::NONE};
  EXPECT_FALSE(CServiceManifest::Load(name, manifest, &error));
  EXPECT_EQ(Error::READ_FAILED, error);
}
