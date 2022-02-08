/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace XFILE::IPFS
{
enum class UnixFSJsonNodeType
{
  File,
  Directory,
  Symlink,
};

struct UnixFSJsonLink
{
  std::string cid;
  std::string name;
  uint64_t blockSize{0};
  uint64_t totalSize{0};
};

struct UnixFSJsonNode
{
  UnixFSJsonNodeType type{UnixFSJsonNodeType::File};
  std::vector<uint8_t> data;
  uint64_t fileSize{0};
  std::vector<UnixFSJsonLink> links;
  uint32_t mode{0};
  int64_t mtimeSeconds{0};
  uint32_t mtimeNanos{0};
};
} // namespace XFILE::IPFS
