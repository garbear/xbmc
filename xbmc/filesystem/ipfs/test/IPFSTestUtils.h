/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "crypto/multiformats/Multihash.h"
#include "datastore/CID.h"

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include <gtest/gtest.h>

namespace XFILE::IPFS::TEST
{
inline std::vector<uint8_t> MakeSha2_256Multihash(const uint8_t* data, size_t size)
{
  KODI::CRYPTO::CMultihash cryptoMultihash{KODI::CRYPTO::CMultihash::Identifier::SHA2_256, {}};
  cryptoMultihash.Update(data, size);
  cryptoMultihash.Finalize();

  std::vector<uint8_t> serialized;
  EXPECT_TRUE(cryptoMultihash.Serialize(serialized));
  return serialized;
}

inline std::vector<uint8_t> MakeSha2_256Multihash(const std::vector<uint8_t>& data)
{
  return MakeSha2_256Multihash(data.data(), data.size());
}

inline KODI::DATASTORE::CCID MakeTestCID(KODI::DATASTORE::CIDCodec codec,
                                         const std::vector<uint8_t>& data)
{
  return KODI::DATASTORE::CCID(codec, MakeSha2_256Multihash(data));
}
} // namespace XFILE::IPFS::TEST
