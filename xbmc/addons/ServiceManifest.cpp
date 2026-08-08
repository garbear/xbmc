/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceManifest.h"

#include "filesystem/File.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <utility>

namespace
{
using Error = ADDON::CServiceManifest::Error;

constexpr std::size_t READ_BUFFER_SIZE = 16 * 1024;

bool Fail(Error* error, Error value) noexcept
{
  if (error != nullptr)
    *error = value;

  return false;
}

Error ParseVersion1(const CVariant& document,
                    std::string& id,
                    std::string& name,
                    std::string& catalog)
{
  if (!document.isMember("id"))
    return Error::MISSING_ID;

  const CVariant& idValue = document["id"];
  if (!idValue.isString())
    return Error::INVALID_ID_TYPE;

  id = idValue.asString();
  if (id.empty())
    return Error::EMPTY_ID;

  if (!document.isMember("name"))
    return Error::MISSING_NAME;

  const CVariant& nameValue = document["name"];
  if (!nameValue.isString())
    return Error::INVALID_NAME_TYPE;

  name = nameValue.asString();
  if (name.empty())
    return Error::EMPTY_NAME;

  if (document.isMember("catalog"))
  {
    const CVariant& catalogValue = document["catalog"];
    if (!catalogValue.isString())
      return Error::INVALID_CATALOG_TYPE;

    catalog = catalogValue.asString();
    if (catalog.empty())
      return Error::EMPTY_CATALOG;
  }

  return Error::NONE;
}
} // namespace

namespace ADDON
{

bool CServiceManifest::Parse(const std::string& json,
                             CServiceManifest& manifest,
                             Error* error) noexcept
try
{
  if (error != nullptr)
    *error = Error::NONE;

  // CJSONVariantParser accepts a null-terminated string. A raw NUL is not valid JSON and would
  // otherwise hide any bytes following it from the parser.
  if (json.find('\0') != std::string::npos)
    return Fail(error, Error::MALFORMED_JSON);

  CVariant document;
  if (!CJSONVariantParser::Parse(json, document))
    return Fail(error, Error::MALFORMED_JSON);

  if (!document.isObject())
    return Fail(error, Error::ROOT_NOT_OBJECT);

  if (!document.isMember("version"))
    return Fail(error, Error::MISSING_VERSION);

  const CVariant& versionValue = document["version"];
  if (!versionValue.isInteger())
    return Fail(error, Error::INVALID_VERSION_TYPE);

  unsigned int version{0};
  if (versionValue.isSignedInteger())
  {
    const int64_t value = versionValue.asInteger();
    if (value < 0 || static_cast<uint64_t>(value) > std::numeric_limits<unsigned int>::max())
      return Fail(error, Error::UNSUPPORTED_VERSION);

    version = static_cast<unsigned int>(value);
  }
  else
  {
    const uint64_t value = versionValue.asUnsignedInteger();
    if (value > std::numeric_limits<unsigned int>::max())
      return Fail(error, Error::UNSUPPORTED_VERSION);

    version = static_cast<unsigned int>(value);
  }

  CServiceManifest parsedManifest;
  parsedManifest.m_version = version;

  Error parseError{Error::NONE};
  switch (version)
  {
    case 1:
      parseError = ParseVersion1(document, parsedManifest.m_id, parsedManifest.m_name,
                                 parsedManifest.m_catalog);
      break;
    default:
      return Fail(error, Error::UNSUPPORTED_VERSION);
  }

  if (parseError != Error::NONE)
    return Fail(error, parseError);

  manifest = std::move(parsedManifest);
  return true;
}
catch (const std::bad_alloc&)
{
  return Fail(error, Error::OUT_OF_MEMORY);
}
catch (...)
{
  return Fail(error, Error::UNKNOWN);
}

bool CServiceManifest::Load(const std::string& uri,
                            CServiceManifest& manifest,
                            Error* error) noexcept
try
{
  if (error != nullptr)
    *error = Error::NONE;

  XFILE::CFile file;
  if (!file.Open(uri, XFILE::READ_TRUNCATED | XFILE::READ_NO_BUFFER))
    return Fail(error, Error::OPEN_FAILED);

  const int64_t reportedLength = file.GetLength();
  if (reportedLength > static_cast<int64_t>(MAX_RESOURCE_SIZE))
    return Fail(error, Error::RESOURCE_TOO_LARGE);

  std::string json;
  if (reportedLength > 0)
    json.reserve(static_cast<std::size_t>(reportedLength));

  std::array<char, READ_BUFFER_SIZE> buffer{};
  while (true)
  {
    if (json.size() == MAX_RESOURCE_SIZE)
    {
      char extraByte{};
      const ssize_t read = file.Read(&extraByte, sizeof(extraByte));
      if (read < 0)
        return Fail(error, Error::READ_FAILED);
      if (read > 0)
        return Fail(error, Error::RESOURCE_TOO_LARGE);

      break;
    }

    const std::size_t bytesToRead = std::min(buffer.size(), MAX_RESOURCE_SIZE - json.size());
    const ssize_t read = file.Read(buffer.data(), bytesToRead);
    if (read < 0)
      return Fail(error, Error::READ_FAILED);
    if (read == 0)
    {
      if (reportedLength > 0 && json.size() < static_cast<std::size_t>(reportedLength))
      {
        return Fail(error, Error::READ_FAILED);
      }

      break;
    }

    json.append(buffer.data(), static_cast<std::size_t>(read));
  }

  file.Close();
  return Parse(json, manifest, error);
}
catch (const std::bad_alloc&)
{
  return Fail(error, Error::OUT_OF_MEMORY);
}
catch (...)
{
  return Fail(error, Error::UNKNOWN);
}

} // namespace ADDON
