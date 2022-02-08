/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UnixFS.h"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

using namespace XFILE::IPFS;

namespace
{
// Magic header for the temporary UnixFS compatibility wire format:
// "KIPFSFB" followed by a one-byte format version.
constexpr std::array<uint8_t, 8> MAGIC{{'K', 'I', 'P', 'F', 'S', 'F', 'B', 1}};

// TODO: Replace this deterministic compatibility encoder with generated
// FlatBuffers code from UnixFS.fbs once schema generation is wired into this
// library target.

void AppendU32(std::vector<uint8_t>& bytes, uint32_t value)
{
  for (unsigned int shift = 0; shift < 32; shift += 8)
    bytes.push_back(static_cast<uint8_t>((value >> shift) & 0xff));
}

void AppendU64(std::vector<uint8_t>& bytes, uint64_t value)
{
  for (unsigned int shift = 0; shift < 64; shift += 8)
    bytes.push_back(static_cast<uint8_t>((value >> shift) & 0xff));
}

void AppendI64(std::vector<uint8_t>& bytes, int64_t value)
{
  AppendU64(bytes, static_cast<uint64_t>(value));
}

bool ReadU32(const uint8_t*& data, size_t& size, uint32_t& value)
{
  if (size < sizeof(uint32_t))
    return false;

  value = 0;
  for (unsigned int shift = 0; shift < 32; shift += 8)
    value |= static_cast<uint32_t>(*data++) << shift;

  size -= sizeof(uint32_t);
  return true;
}

bool ReadU64(const uint8_t*& data, size_t& size, uint64_t& value)
{
  if (size < sizeof(uint64_t))
    return false;

  value = 0;
  for (unsigned int shift = 0; shift < 64; shift += 8)
    value |= static_cast<uint64_t>(*data++) << shift;

  size -= sizeof(uint64_t);
  return true;
}

bool ReadI64(const uint8_t*& data, size_t& size, int64_t& value)
{
  uint64_t raw = 0;
  if (!ReadU64(data, size, raw))
    return false;

  value = static_cast<int64_t>(raw);
  return true;
}

bool ReadBytes(const uint8_t*& data, size_t& size, std::vector<uint8_t>& bytes)
{
  uint64_t byteCount = 0;
  if (!ReadU64(data, size, byteCount))
    return false;

  if (byteCount > size || byteCount > std::numeric_limits<size_t>::max())
    return false;

  bytes.assign(data, data + static_cast<size_t>(byteCount));
  data += byteCount;
  size -= static_cast<size_t>(byteCount);
  return true;
}
} // namespace

bool CUnixFS::EncodeSingleFileNode(const uint8_t* data, size_t size, std::vector<uint8_t>& encoded)
{
  if (data == nullptr && size != 0)
    return false;

  if (size > std::numeric_limits<uint64_t>::max())
    return false;

  std::vector<uint8_t> bytes(MAGIC.begin(), MAGIC.end());
  bytes.push_back(static_cast<uint8_t>(UnixFSNodeType::File));
  AppendU64(bytes, size);
  AppendU64(bytes, size);
  if (data != nullptr && size != 0)
    bytes.insert(bytes.end(), data, data + size);
  AppendU64(bytes, 0); // blocksizes
  AppendU64(bytes, 0); // links
  AppendU32(bytes, 0); // mode
  AppendI64(bytes, 0); // mtime_seconds
  AppendU32(bytes, 0); // mtime_nanos

  encoded = std::move(bytes);
  return true;
}

bool CUnixFS::DecodeNode(const uint8_t* data, size_t size, UnixFSNode& node)
{
  if (data == nullptr || size < MAGIC.size())
    return false;

  if (!std::equal(MAGIC.begin(), MAGIC.end(), data))
    return false;

  const uint8_t* cursor = data + MAGIC.size();
  size -= MAGIC.size();

  if (size < 1)
    return false;

  UnixFSNode decoded;
  decoded.type = static_cast<UnixFSNodeType>(*cursor++);
  --size;

  if (!ReadU64(cursor, size, decoded.fileSize))
    return false;

  if (!ReadBytes(cursor, size, decoded.data))
    return false;

  uint64_t blockSizeCount = 0;
  if (!ReadU64(cursor, size, blockSizeCount))
    return false;
  if (blockSizeCount > size / sizeof(uint64_t))
    return false;
  decoded.blockSizes.reserve(static_cast<size_t>(blockSizeCount));
  for (uint64_t i = 0; i < blockSizeCount; ++i)
  {
    uint64_t blockSize = 0;
    if (!ReadU64(cursor, size, blockSize))
      return false;
    decoded.blockSizes.push_back(blockSize);
  }

  uint64_t linkCount = 0;
  if (!ReadU64(cursor, size, linkCount))
    return false;
  if (linkCount != 0)
    return false;

  if (!ReadU32(cursor, size, decoded.mode))
    return false;
  if (!ReadI64(cursor, size, decoded.mtimeSeconds))
    return false;
  if (!ReadU32(cursor, size, decoded.mtimeNanos))
    return false;
  if (size != 0 || !ValidateNode(decoded))
    return false;

  node = std::move(decoded);
  return true;
}

bool CUnixFS::ValidateNode(const UnixFSNode& node)
{
  if (node.type == UnixFSNodeType::Directory || node.type == UnixFSNodeType::Symlink)
    return false;

  if (node.type != UnixFSNodeType::File)
    return false;

  if (node.blockSizes.size() != node.links.size())
    return false;

  if (!node.links.empty())
    return false;

  return node.fileSize == node.data.size();
}
