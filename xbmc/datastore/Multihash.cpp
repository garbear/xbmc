/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Multihash.h"

using namespace KODI;
using namespace DATASTORE;

CMultihash::CMultihash(MultihashEncoding encoding, std::vector<uint8_t> digest) :
  m_encoding(encoding),
  m_digest(std::move(digest))
{
}

std::vector<uint8_t> CMultihash::Serialize() const
{
  std::vector<uint8_t> data;

  //! @todo

  return data;
}

void CMultihash::Deserialize(const std::vector<uint8_t>& data)
{
  //! @todo
}
