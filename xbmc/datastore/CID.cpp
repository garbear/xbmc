/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CID.h"

using namespace KODI;
using namespace DATASTORE;

CCID::CCID(CIDCodec codec, std::vector<uint8_t> multihash)
  : m_codec(codec), m_multihash(std::move(multihash))
{
}

std::pair<const uint8_t*, size_t> CCID::Serialize() const
{
  return std::make_pair(m_bytes.data(), m_bytes.size());
}

void CCID::Deserialize(const std::vector<uint8_t>& data)
{
  //! @todo
}
