/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace XFILE
{

enum class LeafType {
  FILE,
  RAW
};

enum class CIDVersion {
  CID_V0,
  CID_V1
};

enum class Chunker {
  FIXED_SIZE,
  // ... other chunker types
};

enum class FileLayout {
  BALANCED,
  // ... other layouts
};

struct Mtime {
    int64_t secs; // using int64_t for BigInt equivalent
    int nsecs;
};

struct FileCandidate {
  std::string path;
  std::vector<uint8_t> content;
  Mtime mtime;
  int mode;
};

struct DirectoryCandidate {
  std::string path;
  Mtime mtime;
  int mode;
};

struct File {
  std::vector<uint8_t> content;
  std::string path;
  Mtime mtime;
  int mode;
  std::string originalPath;
};

struct Directory {
  std::string path;
  Mtime mtime;
  int mode;
  std::string originalPath;
};

struct UnixFSOptions {
  std::string type;
  std::vector<uint8_t> data;
  std::vector<int64_t> blockSizes;
  int64_t hashType = 0;
  int64_t fanout = 0;
  Mtime mtime;
  int mode = 0;
};

} // namespace XFILE
