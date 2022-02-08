/*
 *  Copyright (C) 2018-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Block.h"

#include "utils/log.h"

using namespace KODI;
using namespace DATASTORE;

CBlock::CBlock(CCID cid, std::vector<uint8_t> data) : m_cid(std::move(cid))
{
  SetData(std::move(data));
}

CBlock::CBlock(const CBlock& block) : m_cid(block.m_cid)
{
  if (block.Size() == 0)
  {
    SetData(std::vector<uint8_t>{});
    return;
  }

  if (block.Data() == nullptr)
  {
    CLog::Log(LOGERROR, "Block: Cannot copy {} bytes from a null internal data pointer",
              block.Size());
    SetData(std::vector<uint8_t>{});
    return;
  }

  SetData(block.Data(), block.Size());
}

CBlock& CBlock::operator=(const CBlock& block)
{
  if (this == &block)
    return *this;

  m_cid = block.m_cid;

  if (block.Size() == 0)
  {
    SetData(std::vector<uint8_t>{});
    return *this;
  }

  if (block.Data() == nullptr)
  {
    CLog::Log(LOGERROR, "Block: Cannot copy-assign {} bytes from a null internal data pointer",
              block.Size());
    SetData(std::vector<uint8_t>{});
    return *this;
  }

  SetData(block.Data(), block.Size());
  return *this;
}

CBlock CBlock::FromBytes(CCID cid, const uint8_t* data, size_t size)
{
  CBlock block;
  block.SetCID(std::move(cid));
  block.SetData(data, size);
  return block;
}

CBlock CBlock::ViewBytes(CCID cid, const uint8_t* data, size_t size)
{
  CBlock block;
  block.SetCID(std::move(cid));
  block.SetDataView(data, size);
  return block;
}

const uint8_t* CBlock::Data() const
{
  if (m_storageMode == StorageMode::View)
    return m_viewData;

  return m_data.data();
}

size_t CBlock::Size() const
{
  if (m_storageMode == StorageMode::View)
    return m_viewSize;

  return m_data.size();
}

bool CBlock::IsOwning() const
{
  return m_storageMode == StorageMode::Owning;
}

void CBlock::SetData(std::vector<uint8_t> data)
{
  m_storageMode = StorageMode::Owning;
  m_data = std::move(data);
  m_viewData = nullptr;
  m_viewSize = 0;
}

void CBlock::SetData(const uint8_t* data, size_t size)
{
  if (data == nullptr)
  {
    if (size != 0)
    {
      CLog::Log(LOGERROR, "Block: Cannot copy {} bytes from a null data pointer", size);
      return;
    }

    SetData(std::vector<uint8_t>{});
    return;
  }

  SetData(std::vector<uint8_t>(data, data + size));
}

void CBlock::SetDataView(const uint8_t* data, size_t size)
{
  if (data == nullptr && size != 0)
  {
    CLog::Log(LOGERROR, "Block: Cannot view {} bytes from a null data pointer", size);
    return;
  }

  m_storageMode = StorageMode::View;
  m_data.clear();
  m_viewData = data;
  m_viewSize = size;
}

std::pair<const uint8_t*, size_t> CBlock::Serialize() const
{
  return std::make_pair(Data(), Size());
}

void CBlock::Deserialize(const std::vector<uint8_t>& data)
{
  SetData(data);
}

void CBlock::Deserialize(const uint8_t* data, size_t size)
{
  SetData(data, size);
}
