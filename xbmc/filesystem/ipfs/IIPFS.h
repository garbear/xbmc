/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
class IIPFS
{
public:
  virtual ~IIPFS() = default;

  // Lifecycle functions
  virtual bool Initialize(const std::string& dataStoreRoot) = 0;
  virtual void Deinitialize() = 0;
  virtual bool IsOnline() = 0;

  // Name subsystem (TODO)
  virtual std::string ResolveName(const std::string& identifier) { return ""; }
  virtual void PublishName(const std::string& ipfsPath,
                           unsigned int lifetimeSecs,
                           unsigned int ttlSecs,
                           const std::string& keyName) { }

  // Directed Acyclic Graph (DAG) subsystem
  virtual CVariant GetDAG(const std::string& cid) = 0;
  virtual std::string PutDAG(const CVariant& content) = 0;

  // Key subsystem (TODO)
  virtual std::vector<std::string> ListKeys() { return {}; }
  virtual void RemoveKey(const std::string& keyName) { }
  virtual void ImportKey(const std::string& keyName,
                         const KODI::CRYPTO::PrivateKey& privateKey,
                         const std::string& password) { }
};

} // namespace XFILE
