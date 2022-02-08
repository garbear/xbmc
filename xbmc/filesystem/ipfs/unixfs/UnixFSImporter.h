/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace KODI::DATASTORE
{
class CBlockStore;
}

namespace XFILE::IPFS::UNIXFS
{
class CImporter
{
public:
  static bool AddFile(KODI::DATASTORE::CBlockStore& blockStore,
                      const uint8_t* data,
                      size_t size,
                      std::string& cidString);
};
} // namespace XFILE::IPFS::UNIXFS
