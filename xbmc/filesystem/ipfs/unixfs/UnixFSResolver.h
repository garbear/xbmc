/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "UnixFSJsonTypes.h"
#include "datastore/CID.h"

#include <string>
#include <vector>

namespace KODI::DATASTORE
{
class CBlockStore;
}

namespace XFILE::IPFS::UNIXFS
{
enum class ResolvedNodeType
{
  Unknown,
  File,
  Directory,
  Symlink,
};

class CResolver
{
public:
  static bool ReadFile(KODI::DATASTORE::CBlockStore& blockStore,
                       const std::string& cid,
                       std::vector<uint8_t>& data);

  static ResolvedNodeType ResolveNodeType(KODI::DATASTORE::CBlockStore& blockStore,
                                          const std::string& cid);

  static bool ReadNode(KODI::DATASTORE::CBlockStore& blockStore,
                       const std::string& cid,
                       UnixFSJsonNode& node,
                       KODI::DATASTORE::CCID* parsedCid = nullptr);
};
} // namespace XFILE::IPFS::UNIXFS
