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
#include <vector>

namespace ADDON
{

/*!
 * \brief Versioned catalog of media resources provided by a service
 */
class CServiceCatalog
{
public:
  /*!
   * \brief Media item declared by a service catalog
   */
  struct Item
  {
    std::string id;
    std::string name;
    std::string media;
  };

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
    MISSING_ITEMS,
    INVALID_ITEMS_TYPE,
    INVALID_ITEM_TYPE,
    MISSING_ITEM_ID,
    INVALID_ITEM_ID_TYPE,
    EMPTY_ITEM_ID,
    MISSING_ITEM_NAME,
    INVALID_ITEM_NAME_TYPE,
    EMPTY_ITEM_NAME,
    MISSING_ITEM_MEDIA,
    INVALID_ITEM_MEDIA_TYPE,
    EMPTY_ITEM_MEDIA,
    OUT_OF_MEMORY,
    UNKNOWN,
  };

  /*! \brief Maximum catalog resource size accepted by Kodi's loader.
   *
   * This is a Kodi resource limit, not part of the service catalog protocol.
   */
  static constexpr std::size_t MAX_RESOURCE_SIZE = 4 * 1024 * 1024;

  static bool Parse(const std::string& json,
                    CServiceCatalog& catalog,
                    Error* error = nullptr) noexcept;
  static bool Load(const std::string& uri,
                   CServiceCatalog& catalog,
                   Error* error = nullptr) noexcept;

  unsigned int Version() const { return m_version; }
  const std::vector<Item>& Items() const { return m_items; }

private:
  unsigned int m_version{0};
  std::vector<Item> m_items;
};

} // namespace ADDON
