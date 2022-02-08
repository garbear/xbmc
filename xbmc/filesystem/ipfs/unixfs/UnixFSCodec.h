/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "datastore/CID.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace XFILE::IPFS::UNIXFS
{
class CCodec
{
public:
  static bool EncodeRootPayload(const std::vector<uint8_t>& json,
                                std::vector<uint8_t>& payload,
                                KODI::DATASTORE::CIDCodec& codec);

  static bool DecodeRootPayload(KODI::DATASTORE::CIDCodec codec,
                                const uint8_t* data,
                                size_t size,
                                std::vector<uint8_t>& json);

  static bool IsRootCodec(KODI::DATASTORE::CIDCodec codec);
};
} // namespace XFILE::IPFS::UNIXFS
