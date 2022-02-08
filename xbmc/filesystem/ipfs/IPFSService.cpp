/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPFSService.h"

#include "IPFS.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <utility>

using namespace XFILE;

CIPFSService::CIPFSService() = default;

CIPFSService::~CIPFSService()
{
  Deinitialize();
}

bool CIPFSService::Initialize(const CProfileManager& profileManager)
{
  const std::string dataStoreRoot = profileManager.GetDataStoreFolder();

  return Initialize(dataStoreRoot);
}

bool CIPFSService::Initialize(const std::string& dataStoreRoot)
{
  if (dataStoreRoot.empty())
  {
    CLog::Log(LOGERROR, "IPFS: Failed to initialize service, datastore root is empty");
    return false;
  }

  std::unique_lock<std::mutex> lock(m_mutex);

  if (m_ipfs)
  {
    if (m_dataStoreRoot == dataStoreRoot)
      return true;

    CLog::Log(LOGERROR,
              "IPFS: Failed to initialize, service is already initialized with datastore '{}'",
              m_dataStoreRoot);
    return false;
  }

  // Create the datastore root if it doesn't exist
  if (!XFILE::CDirectory::Exists(dataStoreRoot, true) && !XFILE::CDirectory::Create(dataStoreRoot))
  {
    CLog::Log(LOGERROR, "IPFS: Failed to create datastore root '{}'", dataStoreRoot);
    return false;
  }

  std::unique_ptr<CIPFS> ipfs = std::make_unique<CIPFS>();
  if (!ipfs->Initialize(dataStoreRoot))
  {
    CLog::Log(LOGERROR, "IPFS: Failed to initialize datastore at '{}'", dataStoreRoot);
    return false;
  }

  m_ipfs = std::move(ipfs);
  m_dataStoreRoot = dataStoreRoot;

  return true;
}

void CIPFSService::Deinitialize()
{
  std::unique_lock<std::mutex> lock(m_mutex);

  if (m_ipfs)
  {
    m_ipfs->Deinitialize();
    m_ipfs.reset();
  }
  m_dataStoreRoot.clear();
}

bool CIPFSService::IsInitialized() const
{
  std::unique_lock<std::mutex> lock(m_mutex);

  return static_cast<bool>(m_ipfs);
}

bool CIPFSService::AddFile(const uint8_t* data, size_t size, std::string& cid)
{
  if (data == nullptr && size != 0)
    return false;

  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_ipfs)
    return false;

  std::string newCid;
  if (!m_ipfs->AddFile(data, size, newCid))
    return false;

  cid = std::move(newCid);
  return true;
}

bool CIPFSService::GetFile(const std::string& cid, std::vector<uint8_t>& data)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_ipfs)
    return false;

  std::vector<uint8_t> output;
  if (!m_ipfs->GetFile(cid, output))
    return false;

  data = std::move(output);
  return true;
}

bool CIPFSService::HasFile(const std::string& cid)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_ipfs)
    return false;

  std::vector<uint8_t> data;
  return m_ipfs->GetFile(cid, data);
}

bool CIPFSService::IsDirectory(const std::string& cid)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_ipfs)
    return false;

  return m_ipfs->IsDirectory(cid);
}

bool CIPFSService::ListDirectory(const std::string& cid, std::vector<CIPFSEntry>& entries)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_ipfs)
    return false;

  std::vector<CIPFSEntry> output;
  if (!m_ipfs->ListDirectory(cid, output))
    return false;

  entries = std::move(output);
  return true;
}
