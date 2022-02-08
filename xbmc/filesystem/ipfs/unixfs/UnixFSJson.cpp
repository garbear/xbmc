/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UnixFSJson.h"

#include "datastore/CID.h"
#include "filesystem/ipfs/ipld/IPLDJson.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <utility>

using namespace XFILE::IPFS;
using namespace KODI;

namespace
{
namespace JsonKeys
{
constexpr std::string_view TYPE = "type";
constexpr std::string_view DATA = "data";
constexpr std::string_view FILESIZE = "filesize";
constexpr std::string_view LINKS = "links";
constexpr std::string_view MODE = "mode";
constexpr std::string_view MTIME = "mtime";
constexpr std::string_view SECONDS = "seconds";
constexpr std::string_view NANOS = "nanos";
constexpr std::string_view NAME = "name";
constexpr std::string_view LINK = "link";
constexpr std::string_view BLOCK_SIZE = "blockSize";
constexpr std::string_view T_SIZE = "tSize";
} // namespace JsonKeys

namespace JsonTypes
{
constexpr std::string_view FILE = "File";
constexpr std::string_view DIRECTORY = "Directory";
constexpr std::string_view SYMLINK = "Symlink";
} // namespace JsonTypes

bool AddNoOverflow(uint64_t lhs, uint64_t rhs, uint64_t& result)
{
  if (rhs > std::numeric_limits<uint64_t>::max() - lhs)
    return false;

  result = lhs + rhs;
  return true;
}

std::string_view ToJsonType(UnixFSJsonNodeType type)
{
  switch (type)
  {
    case UnixFSJsonNodeType::File:
      return JsonTypes::FILE;
    case UnixFSJsonNodeType::Directory:
      return JsonTypes::DIRECTORY;
    case UnixFSJsonNodeType::Symlink:
      return JsonTypes::SYMLINK;
  }

  return JsonTypes::FILE;
}

bool FromJsonType(const std::string& type, UnixFSJsonNodeType& nodeType)
{
  if (type == JsonTypes::FILE)
  {
    nodeType = UnixFSJsonNodeType::File;
    return true;
  }
  if (type == JsonTypes::DIRECTORY)
  {
    nodeType = UnixFSJsonNodeType::Directory;
    return true;
  }
  if (type == JsonTypes::SYMLINK)
  {
    nodeType = UnixFSJsonNodeType::Symlink;
    return true;
  }

  return false;
}

class CUnixFSJsonValidator
{
public:
  static bool ValidateNode(const UnixFSJsonNode& node)
  {
    switch (node.type)
    {
      case UnixFSJsonNodeType::File:
        return ValidateFile(node);
      case UnixFSJsonNodeType::Directory:
        return ValidateDirectory(node);
      case UnixFSJsonNodeType::Symlink:
        return ValidateSymlink(node);
    }

    return false;
  }

private:
  static bool ValidateFile(const UnixFSJsonNode& node)
  {
    uint64_t totalSize = node.data.size();
    for (const UnixFSJsonLink& link : node.links)
    {
      DATASTORE::CCID parsed;
      if (link.cid.empty() || !link.name.empty() || link.blockSize == 0 ||
          !DATASTORE::CCID::FromString(link.cid, parsed) ||
          parsed.Codec() != DATASTORE::CIDCodec::RAW ||
          !AddNoOverflow(totalSize, link.blockSize, totalSize))
        return false;
    }
    return node.fileSize == totalSize;
  }

  static bool ValidateDirectory(const UnixFSJsonNode& node)
  {
    if (!node.data.empty() || node.fileSize != 0)
      return false;

    std::set<std::string> names;
    for (const UnixFSJsonLink& link : node.links)
    {
      DATASTORE::CCID parsed;
      if (link.cid.empty() || !DATASTORE::CCID::FromString(link.cid, parsed) || link.name.empty() ||
          !names.insert(link.name).second)
        return false;
    }
    return true;
  }

  static bool ValidateSymlink(const UnixFSJsonNode& node)
  {
    return node.links.empty() && node.fileSize == node.data.size();
  }
};

class CUnixFSJsonWriter
{
public:
  static bool EncodeNode(const UnixFSJsonNode& node, std::vector<uint8_t>& encoded)
  {
    if (!CUnixFSJsonValidator::ValidateNode(node))
      return false;

    std::string json;
    json.reserve(node.data.size() + (node.links.size() * 128) + 128);
    AppendNode(json, node);

    encoded.assign(json.begin(), json.end());
    return true;
  }

private:
  static void AppendNode(std::string& json, const UnixFSJsonNode& node)
  {
    json += "{";
    IPLD::CJson::AppendString(json, JsonKeys::TYPE);
    json += ":";
    IPLD::CJson::AppendString(json, ToJsonType(node.type));
    json += ",";

    if (node.type == UnixFSJsonNodeType::Directory)
      AppendDirectoryNode(json, node);
    else if (node.type == UnixFSJsonNodeType::Symlink)
      AppendSymlinkNode(json, node);
    else
      AppendFileNode(json, node);

    AppendMtime(json, node);
    json += "}";
  }

  static void AppendFileNode(std::string& json, const UnixFSJsonNode& node)
  {
    IPLD::CJson::AppendString(json, JsonKeys::DATA);
    json += ":";
    IPLD::CJson::AppendBytes(json, node.data);
    json += ",";
    IPLD::CJson::AppendString(json, JsonKeys::FILESIZE);
    json += ":";
    json += std::to_string(node.fileSize);
    json += ",";
    IPLD::CJson::AppendString(json, JsonKeys::LINKS);
    json += ":";
    AppendFileLinks(json, node.links);
  }

  static void AppendDirectoryNode(std::string& json, const UnixFSJsonNode& node)
  {
    IPLD::CJson::AppendString(json, JsonKeys::LINKS);
    json += ":";
    AppendDirectoryLinks(json, node.links);
  }

  static void AppendSymlinkNode(std::string& json, const UnixFSJsonNode& node)
  {
    AppendFileNode(json, node);
  }

  static void AppendFileLinks(std::string& json, const std::vector<UnixFSJsonLink>& links)
  {
    json += "[";
    for (size_t i = 0; i < links.size(); ++i)
    {
      if (i != 0)
        json += ",";
      json += "{";
      IPLD::CJson::AppendString(json, JsonKeys::LINK);
      json += ":";
      IPLD::CJson::AppendLink(json, links[i].cid);
      json += ",";
      IPLD::CJson::AppendString(json, JsonKeys::BLOCK_SIZE);
      json += ":";
      json += std::to_string(links[i].blockSize);
      json += ",";
      IPLD::CJson::AppendString(json, JsonKeys::T_SIZE);
      json += ":";
      json += std::to_string(links[i].totalSize);
      json += "}";
    }
    json += "]";
  }

  static void AppendDirectoryLinks(std::string& json, std::vector<UnixFSJsonLink> links)
  {
    std::sort(links.begin(), links.end(), [](const UnixFSJsonLink& lhs, const UnixFSJsonLink& rhs)
              { return lhs.name < rhs.name; });

    json += "[";
    for (size_t i = 0; i < links.size(); ++i)
    {
      if (i != 0)
        json += ",";
      json += "{";
      IPLD::CJson::AppendString(json, JsonKeys::NAME);
      json += ":";
      IPLD::CJson::AppendString(json, links[i].name);
      json += ",";
      IPLD::CJson::AppendString(json, JsonKeys::LINK);
      json += ":";
      IPLD::CJson::AppendLink(json, links[i].cid);
      json += ",";
      IPLD::CJson::AppendString(json, JsonKeys::T_SIZE);
      json += ":";
      json += std::to_string(links[i].totalSize);
      json += "}";
    }
    json += "]";
  }

  static void AppendMtime(std::string& json, const UnixFSJsonNode& node)
  {
    json += ",";
    IPLD::CJson::AppendString(json, JsonKeys::MODE);
    json += ":";
    json += std::to_string(node.mode);
    json += ",";
    IPLD::CJson::AppendString(json, JsonKeys::MTIME);
    json += ":{";
    IPLD::CJson::AppendString(json, JsonKeys::SECONDS);
    json += ":";
    json += std::to_string(node.mtimeSeconds);
    json += ",";
    IPLD::CJson::AppendString(json, JsonKeys::NANOS);
    json += ":";
    json += std::to_string(node.mtimeNanos);
    json += "}";
  }
};

class CUnixFSJsonReader
{
public:
  static bool DecodeNode(const uint8_t* data, size_t size, UnixFSJsonNode& node)
  {
    if (data == nullptr || size == 0)
      return false;

    CVariant parsed;
    const std::string json(reinterpret_cast<const char*>(data), size);
    if (!CJSONVariantParser::Parse(json, parsed) || !parsed.isObject() ||
        !IPLD::CJson::HasOnlyKeys(parsed, TopLevelKeys()))
      return false;

    UnixFSJsonNode decoded;
    if (!DecodeType(parsed, decoded) || !DecodeMtime(parsed, decoded))
      return false;

    if (decoded.type == UnixFSJsonNodeType::Directory)
    {
      if (IPLD::CJson::HasMember(parsed, JsonKeys::DATA) ||
          IPLD::CJson::HasMember(parsed, JsonKeys::FILESIZE))
        return false;
    }
    else if (!DecodeData(parsed, decoded))
    {
      return false;
    }

    if (!DecodeLinks(parsed, decoded) || !CUnixFSJsonValidator::ValidateNode(decoded))
      return false;

    node = std::move(decoded);
    return true;
  }

private:
  static constexpr std::array<std::string_view, 6> TopLevelKeys()
  {
    return {JsonKeys::TYPE,  JsonKeys::DATA, JsonKeys::FILESIZE,
            JsonKeys::LINKS, JsonKeys::MODE, JsonKeys::MTIME};
  }

  static constexpr std::array<std::string_view, 2> MtimeKeys()
  {
    return {JsonKeys::SECONDS, JsonKeys::NANOS};
  }

  static constexpr std::array<std::string_view, 3> FileLinkKeys()
  {
    return {JsonKeys::LINK, JsonKeys::BLOCK_SIZE, JsonKeys::T_SIZE};
  }

  static constexpr std::array<std::string_view, 3> DirectoryLinkKeys()
  {
    return {JsonKeys::NAME, JsonKeys::LINK, JsonKeys::T_SIZE};
  }

  static bool DecodeType(const CVariant& object, UnixFSJsonNode& node)
  {
    std::string type;
    return IPLD::CJson::ReadString(object, JsonKeys::TYPE, type) && FromJsonType(type, node.type);
  }

  static bool DecodeMtime(const CVariant& object, UnixFSJsonNode& node)
  {
    uint64_t mode = 0;
    if (!IPLD::CJson::ReadUInt(object, JsonKeys::MODE, mode) ||
        mode > std::numeric_limits<uint32_t>::max())
      return false;
    node.mode = static_cast<uint32_t>(mode);

    if (!IPLD::CJson::HasMember(object, JsonKeys::MTIME) ||
        !IPLD::CJson::Member(object, JsonKeys::MTIME).isObject() ||
        !IPLD::CJson::HasOnlyKeys(IPLD::CJson::Member(object, JsonKeys::MTIME), MtimeKeys()))
      return false;

    int64_t seconds = 0;
    uint64_t nanos = 0;
    if (!IPLD::CJson::ReadInt(IPLD::CJson::Member(object, JsonKeys::MTIME), JsonKeys::SECONDS,
                              seconds) ||
        !IPLD::CJson::ReadUInt(IPLD::CJson::Member(object, JsonKeys::MTIME), JsonKeys::NANOS,
                               nanos) ||
        nanos > std::numeric_limits<uint32_t>::max())
      return false;

    node.mtimeSeconds = seconds;
    node.mtimeNanos = static_cast<uint32_t>(nanos);
    return true;
  }

  static bool DecodeData(const CVariant& object, UnixFSJsonNode& node)
  {
    uint64_t fileSize = 0;
    if (!IPLD::CJson::HasMember(object, JsonKeys::DATA) ||
        !IPLD::CJson::ReadUInt(object, JsonKeys::FILESIZE, fileSize) ||
        !IPLD::CJson::DecodeBytes(IPLD::CJson::Member(object, JsonKeys::DATA), node.data))
      return false;

    node.fileSize = fileSize;
    return true;
  }

  static bool DecodeLinks(const CVariant& object, UnixFSJsonNode& node)
  {
    if (!IPLD::CJson::HasMember(object, JsonKeys::LINKS) ||
        !IPLD::CJson::Member(object, JsonKeys::LINKS).isArray())
      return false;

    for (auto it = IPLD::CJson::Member(object, JsonKeys::LINKS).begin_array();
         it != IPLD::CJson::Member(object, JsonKeys::LINKS).end_array(); ++it)
    {
      if (!it->isObject())
        return false;

      UnixFSJsonLink link;
      if (node.type == UnixFSJsonNodeType::Directory)
      {
        if (!IPLD::CJson::HasOnlyKeys(*it, DirectoryLinkKeys()) ||
            !IPLD::CJson::ReadString(*it, JsonKeys::NAME, link.name))
          return false;
      }
      else if (!IPLD::CJson::HasOnlyKeys(*it, FileLinkKeys()))
      {
        return false;
      }

      if (!IPLD::CJson::HasMember(*it, JsonKeys::LINK) ||
          !IPLD::CJson::DecodeLink(IPLD::CJson::Member(*it, JsonKeys::LINK), link.cid))
        return false;

      uint64_t totalSize = 0;
      if (!IPLD::CJson::ReadUInt(*it, JsonKeys::T_SIZE, totalSize))
        return false;
      link.totalSize = totalSize;

      if (node.type != UnixFSJsonNodeType::Directory)
      {
        uint64_t blockSize = 0;
        if (!IPLD::CJson::ReadUInt(*it, JsonKeys::BLOCK_SIZE, blockSize))
          return false;
        link.blockSize = blockSize;
      }

      node.links.emplace_back(std::move(link));
    }

    return true;
  }
};
} // namespace

bool CUnixFSJson::EncodeSingleFileNode(const uint8_t* data,
                                       size_t size,
                                       std::vector<uint8_t>& encoded)
{
  if (data == nullptr && size != 0)
    return false;

  UnixFSJsonNode node;
  node.type = UnixFSJsonNodeType::File;
  node.fileSize = static_cast<uint64_t>(size);
  if (data != nullptr && size != 0)
    node.data.assign(data, data + size);

  return EncodeNode(node, encoded);
}

bool CUnixFSJson::EncodeNode(const UnixFSJsonNode& node, std::vector<uint8_t>& encoded)
{
  return CUnixFSJsonWriter::EncodeNode(node, encoded);
}

bool CUnixFSJson::DecodeNode(const uint8_t* data, size_t size, UnixFSJsonNode& node)
{
  return CUnixFSJsonReader::DecodeNode(data, size, node);
}

bool CUnixFSJson::ValidateNode(const UnixFSJsonNode& node)
{
  return CUnixFSJsonValidator::ValidateNode(node);
}
