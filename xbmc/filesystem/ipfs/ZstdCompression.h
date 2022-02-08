/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace XFILE::IPFS
{
bool CompressZstd(const uint8_t* data, size_t size, std::vector<uint8_t>& compressed);
bool DecompressZstd(const uint8_t* data, size_t size, std::vector<uint8_t>& decompressed);
} // namespace XFILE::IPFS
