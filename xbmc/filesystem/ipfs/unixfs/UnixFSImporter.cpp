/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UnixFSImporter.h"

#include "UnixFSCodec.h"
#include "UnixFSJson.h"
#include "datastore/CID.h"
#include "filesystem/ipfs/block/IPFSBlockUtils.h"

#include <algorithm>
#include <utility>
#include <vector>

using namespace KODI;
using namespace XFILE::IPFS::UNIXFS;

namespace
{
constexpr size_t UNIXFS_JSON_INLINE_MAX_SIZE = 256 * 1024;
constexpr size_t UNIXFS_JSON_CHUNK_SIZE = 256 * 1024;
} // namespace

bool CImporter::AddFile(DATASTORE::CBlockStore& blockStore,
                        const uint8_t* data,
                        size_t size,
                        std::string& cidString)
{
  if (data == nullptr && size != 0)
    return false;

  XFILE::IPFS::UnixFSJsonNode node;
  node.type = XFILE::IPFS::UnixFSJsonNodeType::File;
  node.fileSize = static_cast<uint64_t>(size);

  if (size <= UNIXFS_JSON_INLINE_MAX_SIZE)
  {
    if (data != nullptr && size != 0)
      node.data.assign(data, data + size);
  }
  else
  {
    for (size_t offset = 0; offset < size; offset += UNIXFS_JSON_CHUNK_SIZE)
    {
      const size_t chunkSize = std::min(UNIXFS_JSON_CHUNK_SIZE, size - offset);
      DATASTORE::CCID childCid;
      if (!XFILE::IPFS::CIPFSBlockUtils::StoreAddressedBlock(blockStore, DATASTORE::CIDCodec::RAW,
                                                             data + offset, chunkSize, childCid))
        return false;

      const std::string childCidString = childCid.ToString();
      if (childCidString.empty())
        return false;

      XFILE::IPFS::UnixFSJsonLink link;
      link.cid = childCidString;
      link.blockSize = chunkSize;
      link.totalSize = chunkSize;
      node.links.emplace_back(std::move(link));
    }
  }

  std::vector<uint8_t> encoded;
  if (!XFILE::IPFS::CUnixFSJson::EncodeNode(node, encoded))
    return false;

  DATASTORE::CIDCodec codec = DATASTORE::CIDCodec::UNKNOWN;
  std::vector<uint8_t> payload;
  if (!CCodec::EncodeRootPayload(encoded, payload, codec))
    return false;

  DATASTORE::CCID cid;
  if (!XFILE::IPFS::CIPFSBlockUtils::StoreAddressedBlock(blockStore, codec, payload.data(),
                                                         payload.size(), cid))
    return false;

  std::string newCidString = cid.ToString();
  if (newCidString.empty())
    return false;

  cidString = std::move(newCidString);
  return true;
}
