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
#include "utils/URIUtils.h"

#include <memory>
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
  if (!ipfsService.IsDirectory(cid))
    return false;

  std::vector<CIPFSEntry> entries;
  if (!ipfsService.ListDirectory(cid, entries))
    return false;

  for (const CIPFSEntry& entry : entries)
  {
    std::string path = "ipfs://" + entry.cid;
    if (entry.IsDirectory())
    {
      URIUtils::AddSlashAtEnd(path);
      if (!path.empty() && path.back() != '/')
        path += '/';
    }

    CFileItemPtr item = std::make_shared<CFileItem>(entry.name);
    item->SetPath(path);
    item->SetFolder(entry.IsDirectory());
    item->SetSize(static_cast<int64_t>(entry.size));
    items.Add(std::move(item));
  }

  return true;
}

bool CIPFSDirectory::ContainsFiles(const CURL& url)
{
  std::string cid;
  if (!CIPFSUtils::ParseCID(url, cid))
    return false;

  CIPFSService& ipfsService = CServiceBroker::GetIPFSService();
  if (!ipfsService.IsDirectory(cid))
    return false;

  std::vector<CIPFSEntry> entries;
  if (ipfsService.ListDirectory(cid, entries))
    return !entries.empty();

  return false;
}
