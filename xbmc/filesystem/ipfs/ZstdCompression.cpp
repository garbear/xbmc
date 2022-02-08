/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZstdCompression.h"

#include "utils/log.h"

#include <limits>

#include <zstd.h>

using namespace XFILE::IPFS;

bool XFILE::IPFS::CompressZstd(const uint8_t* data, size_t size, std::vector<uint8_t>& compressed)
{
  if (data == nullptr && size != 0)
  {
    CLog::Log(LOGERROR, "IPFS: Cannot zstd-compress {} bytes from a null data pointer", size);
    return false;
  }

  const size_t bound = ZSTD_compressBound(size);
  if (ZSTD_isError(bound))
  {
    CLog::Log(LOGERROR, "IPFS: Failed to calculate zstd compression bound: {}",
              ZSTD_getErrorName(bound));
    return false;
  }

  const uint8_t empty = 0;
  const void* source = size == 0 ? &empty : data;
  std::vector<uint8_t> output(bound);
  const size_t result = ZSTD_compress(output.data(), output.size(), source, size, 0);
  if (ZSTD_isError(result))
  {
    CLog::Log(LOGERROR, "IPFS: Failed to zstd-compress {} bytes: {}", size,
              ZSTD_getErrorName(result));
    return false;
  }

  output.resize(result);
  compressed = std::move(output);
  return true;
}

bool XFILE::IPFS::DecompressZstd(const uint8_t* data,
                                 size_t size,
                                 std::vector<uint8_t>& decompressed)
{
  if (data == nullptr && size != 0)
  {
    CLog::Log(LOGERROR, "IPFS: Cannot zstd-decompress {} bytes from a null data pointer", size);
    return false;
  }

  const uint8_t empty = 0;
  const void* source = size == 0 ? &empty : data;
  const unsigned long long frameSize = ZSTD_getFrameContentSize(source, size);
  if (frameSize == ZSTD_CONTENTSIZE_ERROR)
  {
    CLog::Log(LOGERROR, "IPFS: Invalid zstd frame");
    return false;
  }
  if (frameSize == ZSTD_CONTENTSIZE_UNKNOWN)
  {
    CLog::Log(LOGERROR, "IPFS: Zstd frame has unknown decompressed size");
    return false;
  }
  if (frameSize > std::numeric_limits<size_t>::max())
  {
    CLog::Log(LOGERROR, "IPFS: Zstd frame decompressed size is too large: {}", frameSize);
    return false;
  }

  std::vector<uint8_t> output(static_cast<size_t>(frameSize));
  uint8_t emptyDestination = 0;
  void* destination = output.empty() ? &emptyDestination : output.data();
  const size_t result = ZSTD_decompress(destination, output.size(), source, size);
  if (ZSTD_isError(result))
  {
    CLog::Log(LOGERROR, "IPFS: Failed to zstd-decompress {} bytes: {}", size,
              ZSTD_getErrorName(result));
    return false;
  }
  if (result != output.size())
  {
    CLog::Log(LOGERROR, "IPFS: Zstd decompressed size mismatch: expected {}, got {}", output.size(),
              result);
    return false;
  }

  decompressed = std::move(output);
  return true;
}
