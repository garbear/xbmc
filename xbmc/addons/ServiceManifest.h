/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <string>

namespace ADDON
{

class CServiceManifest
{
public:
  enum class Error
  {
    NONE,
    OPEN_FAILED,
    RESOURCE_TOO_LARGE,
    READ_FAILED,
    MALFORMED_JSON,
    ROOT_NOT_OBJECT,
    MISSING_VERSION,
    INVALID_VERSION_TYPE,
    UNSUPPORTED_VERSION,
    MISSING_ID,
    INVALID_ID_TYPE,
    EMPTY_ID,
    MISSING_NAME,
    INVALID_NAME_TYPE,
    EMPTY_NAME,
    OUT_OF_MEMORY,
    UNKNOWN,
  };

  static constexpr std::size_t MAX_RESOURCE_SIZE = 1024 * 1024;

  static bool Parse(const std::string& json,
                    CServiceManifest& manifest,
                    Error* error = nullptr) noexcept;
  static bool Load(const std::string& uri,
                   CServiceManifest& manifest,
                   Error* error = nullptr) noexcept;

  unsigned int Version() const { return m_version; }
  const std::string& ID() const { return m_id; }
  const std::string& Name() const { return m_name; }

private:
  unsigned int m_version{0};
  std::string m_id;
  std::string m_name;
};

} // namespace ADDON
