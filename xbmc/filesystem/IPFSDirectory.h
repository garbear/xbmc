/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"

namespace XFILE
{

class CIPFSDirectory : public IFileDirectory
{
public:
  CIPFSDirectory();
  ~CIPFSDirectory() override;

  // Implementation of IDirectory via IFileDirectory
  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_ALWAYS; }

  // Implementation of IFileDirectory
  bool ContainsFiles(const CURL& url) override;
};

} // namespace XFILE
