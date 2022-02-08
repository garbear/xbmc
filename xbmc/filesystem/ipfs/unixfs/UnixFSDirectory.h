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

namespace KODI::DATASTORE
{
class CBlockStore;
}

namespace XFILE::IPFS::UNIXFS
{
enum class DirectoryEntryType
{
  Unknown,
  File,
  Directory,
  Symlink,
};

struct DirectoryEntry
{
  std::string name;
  std::string cid;
  DirectoryEntryType type{DirectoryEntryType::Unknown};
  uint64_t totalSize{0};
};

class CDirectory
{
public:
  static bool IsDirectory(KODI::DATASTORE::CBlockStore& blockStore, const std::string& cid);

  static bool ListDirectory(KODI::DATASTORE::CBlockStore& blockStore,
                            const std::string& cid,
                            std::vector<DirectoryEntry>& entries);
};
} // namespace XFILE::IPFS::UNIXFS
