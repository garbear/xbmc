/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Block.h"

using namespace KODI;
using namespace DATASTORE;

CBlock::CBlock(CCID cid, std::vector<uint8_t> data) :
  m_cid(std::move(cid)),
  m_data(std::move(data))
{
}

std::vector<uint8_t> CBlock::Serialize() const
{
  std::vector<uint8_t> data;

  //! @todo

  return data;
}

void CBlock::Deserialize(const std::vector<uint8_t>& data)
{
  //! @todo
}
