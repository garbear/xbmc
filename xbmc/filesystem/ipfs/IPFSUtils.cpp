/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPFSUtils.h"

#include "URL.h"
#include "utils/StringUtils.h"

#include <cstring>

using namespace XFILE;

bool CIPFSUtils::ParseCID(const CURL& url, std::string& cid)
{
  std::string value;

  if (url.IsProtocol("ipfs"))
  {
    value = url.GetHostName();

    std::string path = url.GetFileName();
    if (StringUtils::EndsWith(path, "/"))
      path.pop_back();
    if (!path.empty())
      return false;
  }
  else
  {
    value = url.Get();
    if (StringUtils::StartsWith(value, "ipfs://"))
      value.erase(0, std::strlen("ipfs://"));
    else if (StringUtils::StartsWith(value, "/ipfs/"))
      value.erase(0, std::strlen("/ipfs/"));
    else
      return false;

    if (StringUtils::EndsWith(value, "/"))
      value.pop_back();

    if (value.find('/') != std::string::npos)
      return false;
  }

  if (value.empty())
    return false;

  cid = value;
  return true;
}
