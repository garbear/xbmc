/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IUnixFS.h"

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

//class CVariant;

namespace XFILE
{

class CID {};
class UnixFS {};
class Blockstore {};

struct ImporterOptions {
  bool rawLeaves = true;
  bool reduceSingleLeafToSelf = true;
  LeafType leafType = LeafType::FILE;
  CIDVersion cidVersion = CIDVersion::CID_V1;
  int shardSplitThresholdBytes = 262144;
  int shardFanoutBits = 8;
  int fileImportConcurrency = 50;
  int blockWriteConcurrency = 10;
  bool wrapWithDirectory = false;
  Chunker chunker = Chunker::FIXED_SIZE;
  FileLayout layout = FileLayout::BALANCED;
  // ... other options, possibly function pointers or std::function objects for callback-based options
};

struct ImportResult {
  CID cid;
  std::size_t size;
  std::string path;
  UnixFS unixfs;
};

class UnixFSImporter {
public:
  ~UnixFSImporter() = default;

  ImportResult Import(const FileCandidate& file, std::shared_ptr<Blockstore> blockstore, const ImporterOptions& options);
  ImportResult Import(const DirectoryCandidate& dir, std::shared_ptr<Blockstore> blockstore, const ImporterOptions& options);
  ImportResult Import(const std::vector<uint8_t>& content, std::shared_ptr<Blockstore> blockstore, const ImporterOptions& options);
  ImportResult Import(const std::vector<uint8_t>& bufs, std::shared_ptr<Blockstore> blockstore, const ImporterOptions& options);
};
} // namespace XFILE
