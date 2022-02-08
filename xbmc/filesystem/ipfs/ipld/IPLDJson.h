/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Variant.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace XFILE::IPFS::IPLD
{
class CJson
{
public:
  static void AppendString(std::string& json, std::string_view value);

  static void AppendBytes(std::string& json, const std::vector<uint8_t>& bytes);
  static bool DecodeBytes(const CVariant& value, std::vector<uint8_t>& bytes);

  static void AppendLink(std::string& json, std::string_view cid);
  static bool DecodeLink(const CVariant& value, std::string& cid);

  static bool HasMember(const CVariant& object, std::string_view key);
  static const CVariant& Member(const CVariant& object, std::string_view key);

  template<size_t Size>
  static bool HasOnlyKeys(const CVariant& object, const std::array<std::string_view, Size>& allowed)
  {
    for (auto it = object.begin_map(); it != object.end_map(); ++it)
    {
      if (std::find(allowed.begin(), allowed.end(), it->first) == allowed.end())
        return false;
    }
    return true;
  }

  static bool ReadString(const CVariant& object, std::string_view key, std::string& value);
  static bool ReadUInt(const CVariant& object, std::string_view key, uint64_t& value);
  static bool ReadInt(const CVariant& object, std::string_view key, int64_t& value);
};
} // namespace XFILE::IPFS::IPLD
