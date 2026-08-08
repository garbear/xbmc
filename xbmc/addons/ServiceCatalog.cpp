/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceCatalog.h"

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
using Catalog = ADDON::CServiceCatalog;
using Error = Catalog::Error;

constexpr std::size_t READ_BUFFER_SIZE = 16 * 1024;

bool Fail(Error* error, Error value) noexcept
{
  if (error != nullptr)
    *error = value;

  return false;
}

Error ParseItem(const CVariant& value, Catalog::Item& item)
{
  if (!value.isObject())
    return Error::INVALID_ITEM_TYPE;

  if (!value.isMember("id"))
    return Error::MISSING_ITEM_ID;

  const CVariant& idValue = value["id"];
  if (!idValue.isString())
    return Error::INVALID_ITEM_ID_TYPE;

  item.id = idValue.asString();
  if (item.id.empty())
    return Error::EMPTY_ITEM_ID;

  if (!value.isMember("name"))
    return Error::MISSING_ITEM_NAME;

  const CVariant& nameValue = value["name"];
  if (!nameValue.isString())
    return Error::INVALID_ITEM_NAME_TYPE;

  item.name = nameValue.asString();
  if (item.name.empty())
    return Error::EMPTY_ITEM_NAME;

  if (!value.isMember("media"))
    return Error::MISSING_ITEM_MEDIA;

  const CVariant& mediaValue = value["media"];
  if (!mediaValue.isString())
    return Error::INVALID_ITEM_MEDIA_TYPE;

  item.media = mediaValue.asString();
  if (item.media.empty())
    return Error::EMPTY_ITEM_MEDIA;

  return Error::NONE;
}

Error ParseVersion1(const CVariant& document, std::vector<Catalog::Item>& items)
{
  if (!document.isMember("items"))
    return Error::MISSING_ITEMS;

  const CVariant& itemsValue = document["items"];
  if (!itemsValue.isArray())
    return Error::INVALID_ITEMS_TYPE;

  items.reserve(itemsValue.size());
  for (auto it = itemsValue.begin_array(); it != itemsValue.end_array(); ++it)
  {
    Catalog::Item item;
    const Error error = ParseItem(*it, item);
    if (error != Error::NONE)
      return error;

    items.emplace_back(std::move(item));
  }

  return Error::NONE;
}
} // namespace

namespace ADDON
{

bool CServiceCatalog::Parse(const std::string& json,
                            CServiceCatalog& catalog,
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

  CServiceCatalog parsedCatalog;
  parsedCatalog.m_version = version;

  Error parseError{Error::NONE};
  switch (version)
  {
    case 1:
      parseError = ParseVersion1(document, parsedCatalog.m_items);
      break;
    default:
      return Fail(error, Error::UNSUPPORTED_VERSION);
  }

  if (parseError != Error::NONE)
    return Fail(error, parseError);

  catalog = std::move(parsedCatalog);
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

bool CServiceCatalog::Load(const std::string& uri, CServiceCatalog& catalog, Error* error) noexcept
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
        return Fail(error, Error::READ_FAILED);

      break;
    }

    json.append(buffer.data(), static_cast<std::size_t>(read));
  }

  file.Close();
  return Parse(json, catalog, error);
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
