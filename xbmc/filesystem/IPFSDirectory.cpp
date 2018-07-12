/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPFSDirectory.h"

#include "FileItem.h"
#include "IPFS.h"
#include "URL.h"

using namespace XFILE;

CIPFSDirectory::CIPFSDirectory() = default;

CIPFSDirectory::~CIPFSDirectory() = default;

bool CIPFSDirectory::GetDirectory(const CURL& urlOrig, CFileItemList& items)
{
  //! @todo
  return false;
}

bool CIPFSDirectory::ContainsFiles(const CURL& url)
{
  //! @todo
  return false;
}
