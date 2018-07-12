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
  virtual ~IIPFS();

  // Lifecycle functions
  virtual bool Initialize(const std::string& dataStoreRoot) = 0;
  virtual void Deinitialize() = 0;
  virtual bool IsOnline() = 0;

  // Name subsystem
  virtual std::string ResolveName(const std::string& identifier) = 0;
  virtual void PublishName(const std::string& ipfsPath,
                           unsigned int lifetimeSecs,
                           unsigned int ttlSecs,
                           const std::string& keyName) = 0;

  // Directed Acyclic Graph (DAG) subsystem
  virtual CVariant GetDAG(const std::string& cid) = 0;
  virtual std::string PutDAG(const CVariant& content) = 0;

  // Key subsystem
  virtual std::vector<std::string> ListKeys() = 0;
  virtual void RemoveKey(const std::string& keyName) = 0;
  virtual void ImportKey(const std::string& keyName,
                         const KODI::CRYPTO::PrivateKey& privateKey,
                         const std::string& password) = 0;
};

} // namespace XFILE
