/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CURL;

namespace XFILE
{

class CIPFSUtils
{
public:
  static bool ParseCID(const CURL& url, std::string& cid);
};

} // namespace XFILE
