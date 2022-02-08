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

/*! \brief Supported IPFS UnixFS node kinds. */
enum class UnixFSNodeType : uint8_t
{
  /*! \brief Regular file node. */
  File = 0,

  /*! \brief Directory node containing links to child nodes. */
  Directory = 1,

  /*! \brief Symbolic link node. */
  Symlink = 2,
};

/*! \brief Link entry referenced by a UnixFS directory or HAMT shard. */
struct UnixFSLink
{
  /*! \brief Raw CID bytes for the referenced block. */
  std::vector<uint8_t> cid;

  /*! \brief Human-readable link name. */
  std::string name;

  /*! \brief Size of the linked block in bytes. */
  uint64_t blockSize = 0;

  /*! \brief Aggregated size for the linked subtree in bytes. */
  uint64_t totalSize = 0;
};

/*! \brief Decoded UnixFS node payload and metadata. */
struct UnixFSNode
{
  /*! \brief Semantic node type. */
  UnixFSNodeType type = UnixFSNodeType::File;

  /*! \brief Raw payload bytes for file or symlink data. */
  std::vector<uint8_t> data;

  /*! \brief Logical file size in bytes. */
  uint64_t fileSize = 0;

  /*! \brief Per-child block sizes used for chunked files. */
  std::vector<uint64_t> blockSizes;

  /*! \brief Child links associated with this node. */
  std::vector<UnixFSLink> links;

  /*! \brief POSIX mode bits if present. */
  uint32_t mode = 0;

  /*! \brief Modification time (seconds component, Unix epoch). */
  int64_t mtimeSeconds = 0;

  /*! \brief Modification time nanoseconds component. */
  uint32_t mtimeNanos = 0;
};

} // namespace XFILE::IPFS
