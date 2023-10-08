/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPFSFile.h"

using namespace XFILE;

CIPFSFile::CIPFSFile() = default;

CIPFSFile::~CIPFSFile()
{
  Close();
}

bool CIPFSFile::Open(const CURL& url)
{
  //! @todo
  return false;
}

bool CIPFSFile::Exists(const CURL& url)
{
  //! @todo
  return false;
}

int CIPFSFile::Stat(const CURL& url, struct __stat64* buffer)
{
  //! @todo
  return -1;
}

int CIPFSFile::Stat(struct __stat64* buffer)
{
  //! @todo
  return -1;
}

ssize_t CIPFSFile::Read(void* lpBuf, size_t uiBufSize)
{
  //! @todo
  return -1;
}

ssize_t CIPFSFile::AddContent(const void* bufPtr, size_t bufSize, std::string& contentId)
{
  //! @todo
  return -1;
}

int64_t CIPFSFile::GetPosition()
{
  //! @todo
  return -1;
}

int64_t CIPFSFile::GetLength()
{
  //! @todo
  return -1;
}

int64_t CIPFSFile::Seek(int64_t iFilePosition, int iWhence)
{
  //! @todo
  return -1;
}

void CIPFSFile::Close()
{
  //! @todo
}
