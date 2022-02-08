/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPFSFile.h"

#include "IPFSService.h"
#include "IPFSUtils.h"
#include "ServiceBroker.h"
#include "URL.h"

#include <algorithm>
#include <cstring>
#include <limits>

using namespace XFILE;

namespace
{
bool FillStatBuffer(struct __stat64* buffer, size_t size)
{
  if (buffer == nullptr || size > static_cast<size_t>(std::numeric_limits<int64_t>::max()))
    return false;

  *buffer = {};
  buffer->st_size = static_cast<int64_t>(size);
  buffer->st_mode = _S_IFREG;
  return true;
}
} // namespace

CIPFSFile::CIPFSFile() : m_ipfsService(CServiceBroker::GetIPFSService())
{
}

CIPFSFile::~CIPFSFile()
{
  Close();
}

bool CIPFSFile::Open(const CURL& url)
{
  Close();

  std::string cid;
  if (!CIPFSUtils::ParseCID(url, cid))
    return false;

  std::vector<uint8_t> data;
  if (!m_ipfsService.GetFile(cid, data))
    return false;

  m_data = std::move(data);
  m_cid = std::move(cid);
  m_position = 0;
  m_open = true;
  return true;
}

bool CIPFSFile::Exists(const CURL& url)
{
  std::string cid;
  if (!CIPFSUtils::ParseCID(url, cid))
    return false;

  return m_ipfsService.HasFile(cid);
}

int CIPFSFile::Stat(const CURL& url, struct __stat64* buffer)
{
  if (buffer == nullptr)
    return -1;

  std::string cid;
  if (!CIPFSUtils::ParseCID(url, cid))
    return -1;

  std::vector<uint8_t> data;
  if (!m_ipfsService.GetFile(cid, data) || !FillStatBuffer(buffer, data.size()))
    return -1;

  return 0;
}

int CIPFSFile::Stat(struct __stat64* buffer)
{
  if (!m_open || !FillStatBuffer(buffer, m_data.size()))
    return -1;

  return 0;
}

ssize_t CIPFSFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (!m_open || (lpBuf == nullptr && uiBufSize != 0))
    return -1;

  if (uiBufSize == 0)
    return 0;

  if (m_position < 0)
    return -1;

  const size_t position = static_cast<size_t>(m_position);
  if (position >= m_data.size())
    return 0;

  const size_t remaining = m_data.size() - position;
  const size_t readSize =
      std::min({uiBufSize, remaining, static_cast<size_t>(std::numeric_limits<ssize_t>::max())});
  std::memcpy(lpBuf, m_data.data() + position, readSize);
  m_position += static_cast<int64_t>(readSize);

  return static_cast<ssize_t>(readSize);
}

ssize_t CIPFSFile::AddContent(const void* bufPtr, size_t bufSize, std::string& contentId)
{
  if (bufPtr == nullptr && bufSize != 0)
    return -1;

  if (bufSize > static_cast<size_t>(std::numeric_limits<ssize_t>::max()))
    return -1;

  std::string cid;
  if (!m_ipfsService.AddFile(static_cast<const uint8_t*>(bufPtr), bufSize, cid))
    return -1;

  contentId = std::move(cid);
  return static_cast<ssize_t>(bufSize);
}

int64_t CIPFSFile::GetPosition()
{
  if (!m_open)
    return -1;

  return m_position;
}

int64_t CIPFSFile::GetLength()
{
  if (!m_open || m_data.size() > static_cast<size_t>(std::numeric_limits<int64_t>::max()))
    return -1;

  return static_cast<int64_t>(m_data.size());
}

int64_t CIPFSFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_open || m_data.size() > static_cast<size_t>(std::numeric_limits<int64_t>::max()))
    return -1;

  const int64_t length = static_cast<int64_t>(m_data.size());
  int64_t position = 0;

  switch (iWhence)
  {
    case SEEK_SET:
      position = iFilePosition;
      break;
    case SEEK_CUR:
      if ((iFilePosition > 0 && m_position > std::numeric_limits<int64_t>::max() - iFilePosition) ||
          (iFilePosition < 0 && m_position < std::numeric_limits<int64_t>::min() - iFilePosition))
        return -1;
      position = m_position + iFilePosition;
      break;
    case SEEK_END:
      if ((iFilePosition > 0 && length > std::numeric_limits<int64_t>::max() - iFilePosition) ||
          (iFilePosition < 0 && length < std::numeric_limits<int64_t>::min() - iFilePosition))
        return -1;
      position = length + iFilePosition;
      break;
    default:
      return -1;
  }

  if (position < 0)
    return -1;

  m_position = position;
  return m_position;
}

void CIPFSFile::Close()
{
  m_data.clear();
  m_position = 0;
  m_cid.clear();
  m_open = false;
}
