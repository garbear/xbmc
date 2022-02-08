/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "datastore/CID.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class CVariant;

namespace KODI
{
namespace CRYPTO
{
struct PrivateKey;
} // namespace CRYPTO
} // namespace KODI

namespace XFILE
{
enum class CIPFSEntryType
{
  Unknown,
  File,
  Directory,
};

struct CIPFSEntry
{
  std::string name;
  std::string cid;
  CIPFSEntryType type{CIPFSEntryType::Unknown};
  uint64_t size{0};

  bool IsDirectory() const { return type == CIPFSEntryType::Directory; }
  bool IsFile() const { return type == CIPFSEntryType::File; }
};

class IIPFS
{
public:
  virtual ~IIPFS() = default;

  // Lifecycle functions
  virtual bool Initialize(const std::string& dataStoreRoot) = 0;
  virtual void Deinitialize() = 0;
  virtual bool IsOnline() = 0;

  // Name subsystem
  virtual std::string ResolveName(const std::string& identifier) = 0;
  virtual bool PublishName(const std::string& ipfsPath,
                           unsigned int lifetimeSecs,
                           unsigned int ttlSecs,
                           const std::string& keyName) = 0;

  // Directed Acyclic Graph (DAG) subsystem
  virtual CVariant GetDAG(const std::string& cid) = 0;
  virtual std::string PutDAG(
      const CVariant& content,
      KODI::DATASTORE::CIDCodec codec = KODI::DATASTORE::CIDCodec::DAG_JSON) = 0;

  // Byte-oriented file subsystem
  virtual bool AddFile(const uint8_t* data, size_t size, std::string& cid) = 0;
  virtual bool GetFile(const std::string& cid, std::vector<uint8_t>& data) = 0;

  // Byte-oriented directory subsystem
  virtual bool IsDirectory(const std::string& cid) = 0;
  virtual bool ListDirectory(const std::string& cid, std::vector<CIPFSEntry>& entries) = 0;

  // Key subsystem
  virtual std::vector<std::string> ListKeys() = 0;
  virtual void RemoveKey(const std::string& keyName) = 0;
  virtual void ImportKey(const std::string& keyName,
                         const KODI::CRYPTO::PrivateKey& privateKey,
                         const std::string& password) = 0;
};

} // namespace XFILE
