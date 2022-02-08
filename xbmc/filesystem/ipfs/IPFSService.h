/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IIPFS.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class CProfileManager;
class CServiceManager;

namespace XFILE
{

class CIPFS;

class CIPFSService
{
public:
  CIPFSService();
  ~CIPFSService();

  bool Initialize(const CProfileManager& profileManager);
  bool Initialize(const std::string& dataStoreRoot);
  void Deinitialize();
  bool IsInitialized() const;

  bool AddFile(const uint8_t* data, size_t size, std::string& cid);
  bool GetFile(const std::string& cid, std::vector<uint8_t>& data);
  bool HasFile(const std::string& cid);

  bool IsDirectory(const std::string& cid);
  bool ListDirectory(const std::string& cid, std::vector<CIPFSEntry>& entries);

private:
  mutable std::mutex m_mutex;
  std::unique_ptr<CIPFS> m_ipfs;
  std::string m_dataStoreRoot;
};

} // namespace XFILE
