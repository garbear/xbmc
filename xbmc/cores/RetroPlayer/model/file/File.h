/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
namespace RETRO
{
  class CFileDataObject;
  class CFileHash;

  class CFile
  {
  public:
    CFile() = default;

    std::string fileName;
    uint64_t fileSize = 0;
    std::vector<std::shared_ptr<CFileHash>> hashes;
    std::string hashAlgorithm;
    std::string hashDigest;
  };
}
}
