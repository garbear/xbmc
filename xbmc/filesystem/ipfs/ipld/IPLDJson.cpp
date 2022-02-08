/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPLDJson.h"

#include "datastore/CID.h"
#include "utils/Base64.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <cctype>
#include <utility>

using namespace KODI;
using namespace XFILE::IPFS::IPLD;

namespace
{
constexpr std::string_view IPLD_KEY = "/";
constexpr std::string_view BYTES_KEY = "bytes";

std::string Key(std::string_view key)
{
  return std::string{key};
}

std::string EncodeBase64(const std::vector<uint8_t>& data)
{
  if (data.empty())
    return "";

  return Base64::Encode(reinterpret_cast<const char*>(data.data()),
                        static_cast<unsigned int>(data.size()));
}

bool IsValidBase64(const std::string& value)
{
  if (value.empty())
    return true;
  if (value.size() % 4 != 0)
    return false;

  bool padding = false;
  size_t paddingCount = 0;
  for (const unsigned char ch : value)
  {
    if (ch == '=')
    {
      padding = true;
      ++paddingCount;
      if (paddingCount > 2)
        return false;
      continue;
    }

    if (padding)
      return false;

    if (!std::isalnum(ch) && ch != '+' && ch != '/')
      return false;
  }

  return true;
}

bool DecodeBase64(const std::string& value, std::vector<uint8_t>& data)
{
  if (!IsValidBase64(value))
    return false;

  const std::string decoded = Base64::Decode(value);
  std::vector<uint8_t> bytes(decoded.begin(), decoded.end());
  if (EncodeBase64(bytes) != value)
    return false;

  data = std::move(bytes);
  return true;
}
} // namespace

void CJson::AppendString(std::string& json, std::string_view value)
{
  json.push_back('"');
  for (const unsigned char ch : value)
  {
    switch (ch)
    {
      case '"':
        json += "\\\"";
        break;
      case '\\':
        json += "\\\\";
        break;
      case '\b':
        json += "\\b";
        break;
      case '\f':
        json += "\\f";
        break;
      case '\n':
        json += "\\n";
        break;
      case '\r':
        json += "\\r";
        break;
      case '\t':
        json += "\\t";
        break;
      default:
        if (ch < 0x20)
          json += StringUtils::Format("\\u{:04x}", ch);
        else
          json.push_back(static_cast<char>(ch));
        break;
    }
  }
  json.push_back('"');
}

void CJson::AppendBytes(std::string& json, const std::vector<uint8_t>& bytes)
{
  json += "{\"/\":{\"bytes\":";
  AppendString(json, EncodeBase64(bytes));
  json += "}}";
}

bool CJson::DecodeBytes(const CVariant& value, std::vector<uint8_t>& bytes)
{
  if (!value.isObject() || !HasOnlyKeys(value, std::array<std::string_view, 1>{IPLD_KEY}) ||
      !HasMember(value, IPLD_KEY))
    return false;

  const CVariant& inner = Member(value, IPLD_KEY);
  if (!inner.isObject() || !HasOnlyKeys(inner, std::array<std::string_view, 1>{BYTES_KEY}) ||
      !HasMember(inner, BYTES_KEY) || !Member(inner, BYTES_KEY).isString())
    return false;

  std::vector<uint8_t> decoded;
  if (!DecodeBase64(Member(inner, BYTES_KEY).asString(), decoded))
    return false;

  bytes = std::move(decoded);
  return true;
}

void CJson::AppendLink(std::string& json, std::string_view cid)
{
  json += "{\"/\":";
  AppendString(json, cid);
  json += "}";
}

bool CJson::DecodeLink(const CVariant& value, std::string& cid)
{
  if (!value.isObject() || !HasOnlyKeys(value, std::array<std::string_view, 1>{IPLD_KEY}) ||
      !HasMember(value, IPLD_KEY) || !Member(value, IPLD_KEY).isString())
    return false;

  const std::string decoded = Member(value, IPLD_KEY).asString();
  DATASTORE::CCID parsed;
  if (decoded.empty() || !DATASTORE::CCID::FromString(decoded, parsed))
    return false;

  cid = decoded;
  return true;
}

bool CJson::HasMember(const CVariant& object, std::string_view key)
{
  return object.isMember(Key(key));
}

const CVariant& CJson::Member(const CVariant& object, std::string_view key)
{
  return object[Key(key)];
}

bool CJson::ReadString(const CVariant& object, std::string_view key, std::string& value)
{
  if (!HasMember(object, key) || !Member(object, key).isString())
    return false;

  value = Member(object, key).asString();
  return true;
}

bool CJson::ReadUInt(const CVariant& object, std::string_view key, uint64_t& value)
{
  if (!HasMember(object, key) ||
      (!Member(object, key).isUnsignedInteger() && !Member(object, key).isSignedInteger()))
    return false;

  if (Member(object, key).isSignedInteger())
  {
    const int64_t signedValue = Member(object, key).asInteger();
    if (signedValue < 0)
      return false;
    value = static_cast<uint64_t>(signedValue);
  }
  else
  {
    value = Member(object, key).asUnsignedInteger();
  }
  return true;
}

bool CJson::ReadInt(const CVariant& object, std::string_view key, int64_t& value)
{
  if (!HasMember(object, key) || !Member(object, key).isInteger())
    return false;

  value = Member(object, key).asInteger();
  return true;
}
