/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UnixFSCodec.h"

#include "filesystem/ipfs/ZstdCompression.h"

using namespace KODI;
using namespace XFILE::IPFS::UNIXFS;

namespace
{
constexpr size_t UNIXFS_JSON_ZSTD_MIN_SIZE = 4096;
}

bool CCodec::EncodeRootPayload(const std::vector<uint8_t>& json,
                               std::vector<uint8_t>& payload,
                               DATASTORE::CIDCodec& codec)
{
  DATASTORE::CIDCodec newCodec = DATASTORE::CIDCodec::DAG_JSON;
  std::vector<uint8_t> newPayload = json;

  if (json.size() >= UNIXFS_JSON_ZSTD_MIN_SIZE)
  {
    std::vector<uint8_t> compressed;
    if (XFILE::IPFS::CompressZstd(json.data(), json.size(), compressed) &&
        compressed.size() < json.size())
    {
      newPayload = std::move(compressed);
      newCodec = DATASTORE::CIDCodec::DAG_JSON_ZSTD;
    }
  }

  payload = std::move(newPayload);
  codec = newCodec;
  return true;
}

bool CCodec::DecodeRootPayload(DATASTORE::CIDCodec codec,
                               const uint8_t* data,
                               size_t size,
                               std::vector<uint8_t>& json)
{
  if (!IsRootCodec(codec) || (data == nullptr && size != 0))
    return false;

  std::vector<uint8_t> decoded;
  if (data != nullptr && size != 0)
    decoded.assign(data, data + size);

  if (codec == DATASTORE::CIDCodec::DAG_JSON_ZSTD)
  {
    std::vector<uint8_t> decompressed;
    if (!XFILE::IPFS::DecompressZstd(decoded.data(), decoded.size(), decompressed))
      return false;

    decoded = std::move(decompressed);
  }

  json = std::move(decoded);
  return true;
}

bool CCodec::IsRootCodec(DATASTORE::CIDCodec codec)
{
  return codec == DATASTORE::CIDCodec::DAG_JSON || codec == DATASTORE::CIDCodec::DAG_JSON_ZSTD;
}
