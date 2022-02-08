/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UnixFSDirectory.h"

#include "UnixFSResolver.h"

using namespace KODI;
using namespace XFILE::IPFS::UNIXFS;

namespace
{
DirectoryEntryType ToDirectoryEntryType(ResolvedNodeType type)
{
  switch (type)
  {
    case ResolvedNodeType::File:
      return DirectoryEntryType::File;
    case ResolvedNodeType::Directory:
      return DirectoryEntryType::Directory;
    case ResolvedNodeType::Symlink:
      return DirectoryEntryType::Symlink;
    case ResolvedNodeType::Unknown:
    default:
      return DirectoryEntryType::Unknown;
  }
}
} // namespace

bool CDirectory::IsDirectory(DATASTORE::CBlockStore& blockStore, const std::string& cid)
{
  UnixFSJsonNode node;
  return CResolver::ReadNode(blockStore, cid, node) && node.type == UnixFSJsonNodeType::Directory;
}

bool CDirectory::ListDirectory(DATASTORE::CBlockStore& blockStore,
                               const std::string& cid,
                               std::vector<DirectoryEntry>& entries)
{
  UnixFSJsonNode node;
  if (!CResolver::ReadNode(blockStore, cid, node) || node.type != UnixFSJsonNodeType::Directory)
    return false;

  std::vector<DirectoryEntry> output;
  output.reserve(node.links.size());
  for (const UnixFSJsonLink& link : node.links)
  {
    DirectoryEntry entry;
    entry.name = link.name;
    entry.cid = link.cid;
    entry.type = ToDirectoryEntryType(CResolver::ResolveNodeType(blockStore, link.cid));
    entry.totalSize = link.totalSize;
    output.emplace_back(std::move(entry));
  }

  entries = std::move(output);
  return true;
}
