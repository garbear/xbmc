/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "UnixFSJsonTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace XFILE::IPFS
{
class CUnixFSJson
{
public:
  static bool EncodeSingleFileNode(const uint8_t* data, size_t size, std::vector<uint8_t>& encoded);

  static bool EncodeNode(const UnixFSJsonNode& node, std::vector<uint8_t>& encoded);

  static bool DecodeNode(const uint8_t* data, size_t size, UnixFSJsonNode& node);

  static bool ValidateNode(const UnixFSJsonNode& node);
};
} // namespace XFILE::IPFS
