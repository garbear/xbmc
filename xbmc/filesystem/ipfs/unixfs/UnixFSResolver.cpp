/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UnixFSResolver.h"

#include "UnixFSCodec.h"
#include "UnixFSJson.h"
#include "datastore/Block.h"
#include "datastore/BlockStore.h"

using namespace KODI;
using namespace XFILE::IPFS::UNIXFS;

ResolvedNodeType CResolver::ResolveNodeType(DATASTORE::CBlockStore& blockStore,
                                            const std::string& cid)
{
  DATASTORE::CCID ccid;
  if (!DATASTORE::CCID::FromString(cid, ccid))
    return ResolvedNodeType::Unknown;

  if (ccid.Codec() == DATASTORE::CIDCodec::RAW)
    return blockStore.Has(ccid) ? ResolvedNodeType::File : ResolvedNodeType::Unknown;

  if (!CCodec::IsRootCodec(ccid.Codec()))
    return ResolvedNodeType::Unknown;

  UnixFSJsonNode node;
  if (!ReadNode(blockStore, cid, node))
    return ResolvedNodeType::Unknown;

  switch (node.type)
  {
    case UnixFSJsonNodeType::File:
      return ResolvedNodeType::File;
    case UnixFSJsonNodeType::Directory:
      return ResolvedNodeType::Directory;
    case UnixFSJsonNodeType::Symlink:
      return ResolvedNodeType::Symlink;
    default:
      return ResolvedNodeType::Unknown;
  }
}

bool CResolver::ReadNode(DATASTORE::CBlockStore& blockStore,
                         const std::string& cid,
                         UnixFSJsonNode& node,
                         DATASTORE::CCID* parsedCid)
{
  DATASTORE::CCID ccid;
  if (!DATASTORE::CCID::FromString(cid, ccid) || !CCodec::IsRootCodec(ccid.Codec()))
    return false;

  DATASTORE::CBlock block;
  if (!blockStore.Get(ccid, block))
    return false;

  std::vector<uint8_t> json;
  if (!CCodec::DecodeRootPayload(ccid.Codec(), block.Data(), block.Size(), json))
    return false;

  UnixFSJsonNode decoded;
  if (!XFILE::IPFS::CUnixFSJson::DecodeNode(json.data(), json.size(), decoded))
    return false;

  node = std::move(decoded);
  if (parsedCid != nullptr)
    *parsedCid = std::move(ccid);
  return true;
}

bool CResolver::ReadFile(DATASTORE::CBlockStore& blockStore,
                         const std::string& cid,
                         std::vector<uint8_t>& data)
{
  UnixFSJsonNode node;
  if (!ReadNode(blockStore, cid, node) || node.type != UnixFSJsonNodeType::File)
    return false;

  std::vector<uint8_t> assembled = node.data;
  for (const UnixFSJsonLink& link : node.links)
  {
    if (!link.name.empty() || link.cid.empty())
      return false;

    DATASTORE::CCID childCid;
    if (!DATASTORE::CCID::FromString(link.cid, childCid) ||
        childCid.Codec() != DATASTORE::CIDCodec::RAW)
      return false;

    DATASTORE::CBlock childBlock;
    if (!blockStore.Get(childCid, childBlock) || childBlock.Size() != link.blockSize)
      return false;

    if (childBlock.Data() != nullptr && childBlock.Size() != 0)
      assembled.insert(assembled.end(), childBlock.Data(), childBlock.Data() + childBlock.Size());
  }

  if (assembled.size() != node.fileSize)
    return false;

  data = std::move(assembled);
  return true;
}
