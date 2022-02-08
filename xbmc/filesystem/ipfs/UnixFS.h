/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "UnixFSTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace XFILE::IPFS
{
/**
 * @brief Encodes and decodes UnixFS compatibility nodes
 *
 * This helper provides deterministic serialization and validation for the
 * temporary UnixFS compatibility format used by the IPFS filesystem module.
 */
class CUnixFS
{
public:
  /**
   * @brief Encode a single-file UnixFS node into the compatibility wire format
   *
   * @param data Pointer to file payload bytes. May be null when @p size is zero
   * @param size Number of payload bytes referenced by @p data
   * @param encoded Output buffer receiving the encoded node on success
   *
   * @return True when encoding succeeds and @p encoded contains valid output
   */
  static bool EncodeSingleFileNode(const uint8_t* data, size_t size, std::vector<uint8_t>& encoded);

  /**
   * @brief Decode a compatibility wire-format UnixFS node
   *
   * @param data Pointer to encoded input bytes
   * @param size Number of bytes available at @p data
   * @param node Output node populated when decoding and validation succeed
   *
   * @return True when @p data contains a supported and valid encoded node
   */
  static bool DecodeNode(const uint8_t* data, size_t size, UnixFSNode& node);

  /**
   * @brief Validate decoded UnixFS node invariants used by this module
   *
   * @param node Node to validate
   *
   * @return True when the node satisfies all format and structural constraints
   */
  static bool ValidateNode(const UnixFSNode& node);
};
} // namespace XFILE::IPFS
