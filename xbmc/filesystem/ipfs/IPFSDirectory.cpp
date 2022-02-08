/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPFSDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "IPFSService.h"
#include "IPFSUtils.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "utils/log.h"

#include <vector>

using namespace XFILE;

CIPFSDirectory::CIPFSDirectory() = default;

CIPFSDirectory::~CIPFSDirectory() = default;

bool CIPFSDirectory::GetDirectory(const CURL& urlOrig, CFileItemList& items)
{
  items.Clear();

  std::string cid;
  if (!CIPFSUtils::ParseCID(urlOrig, cid))
    return false;

  CIPFSService& ipfsService = CServiceBroker::GetIPFSService();
  if (!ipfsService.IsDirectory(cid) && ipfsService.HasFile(cid))
    return false;

  std::vector<CIPFSEntry> entries;
  if (ipfsService.ListDirectory(cid, entries))
    return true;

  CLog::Log(LOGDEBUG, "IPFS: Directory listing is not implemented for '{}'", cid);
  return false;
}

bool CIPFSDirectory::ContainsFiles(const CURL& url)
{
  std::string cid;
  if (!CIPFSUtils::ParseCID(url, cid))
    return false;

  CIPFSService& ipfsService = CServiceBroker::GetIPFSService();
  if (!ipfsService.IsDirectory(cid) && ipfsService.HasFile(cid))
    return false;

  std::vector<CIPFSEntry> entries;
  if (ipfsService.ListDirectory(cid, entries))
    return !entries.empty();

  CLog::Log(LOGDEBUG, "IPFS: Directory probing is not implemented for '{}'", cid);
  return false;
}
