/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDInputStreamFile.h"

#include "filesystem/File.h"
#include "filesystem/IFile.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"

#if !defined(TARGET_WINDOWS)
#include "platform/posix/ConvUtils.h"
#endif

#include <cerrno>

using namespace KODI;
using namespace XFILE;

CDVDInputStreamFile::CDVDInputStreamFile(const CFileItem& fileitem, unsigned int flags)
  : CDVDInputStream(DVDSTREAM_TYPE_FILE, fileitem), m_flags(flags)
{
  m_eof = true;
}

CDVDInputStreamFile::~CDVDInputStreamFile()
{
  Close();
}

bool CDVDInputStreamFile::IsEOF()
{
  return !GetFile() || m_eof;
}

bool CDVDInputStreamFile::Open()
{
  if (!CDVDInputStream::Open())
    return false;

  auto file = std::make_shared<CFile>();
  {
    std::unique_lock lock(m_fileSync);
    if (m_abortRequested)
    {
      SetLastError(ECANCELED);
      return false;
    }
    m_pFile = file;
  }

  unsigned int flags = m_flags;

  // If this file is audio and/or video (= not a subtitle) flag to caller
  if (!VIDEO::IsSubtitle(m_item))
    flags |= READ_AUDIO_VIDEO;
  else
    flags |= READ_NO_BUFFER; // disable CFileStreamBuffer for subtitles

  std::string content = m_item.GetMimeType();

  if (content == "video/mp4" ||
      content == "video/x-msvideo" ||
      content == "video/avi" ||
      content == "video/x-matroska" ||
      content == "video/x-matroska-3d")
    flags |= READ_MULTI_STREAM;

  // open file in binary mode
  if (!file->Open(m_item.GetDynPath(), flags))
  {
    std::unique_lock lock(m_fileSync);
    if (m_pFile == file)
      m_pFile.reset();
    return false;
  }

  {
    std::unique_lock lock(m_fileSync);
    if (m_abortRequested)
    {
      if (m_pFile == file)
        m_pFile.reset();
      SetLastError(ECANCELED);
      return false;
    }
  }

  if (file->GetImplementation() && (content.empty() || content == "application/octet-stream"))
    m_content = file->GetImplementation()->GetProperty(XFILE::FileProperty::CONTENT_TYPE);

  m_eof = false;
  return true;
}

std::shared_ptr<CFile> CDVDInputStreamFile::GetFile() const
{
  std::unique_lock lock(m_fileSync);
  return m_pFile;
}

void CDVDInputStreamFile::Abort()
{
  std::shared_ptr<CFile> file;
  {
    std::unique_lock lock(m_fileSync);
    m_abortRequested = true;
    file = m_pFile;
  }

  if (file)
    file->Abort();
}

// close file and reset everything
void CDVDInputStreamFile::Close()
{
  {
    std::shared_ptr<CFile> file;
    {
      std::unique_lock lock(m_fileSync);
      file = std::move(m_pFile);
      m_abortRequested = false;
    }
    if (file)
      file->Close();
  }

  CDVDInputStream::Close();
  m_eof = true;
}

int CDVDInputStreamFile::Read(uint8_t* buf, int buf_size)
{
  auto file = GetFile();
  if (!file)
    return -1;

  ssize_t ret = file->Read(buf, buf_size);

  if (ret < 0)
    return -1; // player will retry read in case of error until playback is stopped

  /* we currently don't support non completing reads */
  if (ret == 0)
    m_eof = true;

  return (int)ret;
}

int64_t CDVDInputStreamFile::Seek(int64_t offset, int whence)
{
  auto file = GetFile();
  if (!file)
    return -1;

  if (whence == DVDSTREAM_SEEK_POSSIBLE)
    return file->IoControl(IOControl::SEEK_POSSIBLE, nullptr);

  int64_t ret = file->Seek(offset, whence);

  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

int64_t CDVDInputStreamFile::GetLength()
{
  if (auto file = GetFile())
    return file->GetLength();
  return 0;
}

bool CDVDInputStreamFile::GetCacheStatus(XFILE::SCacheStatus *status)
{
  auto file = GetFile();
  if (file && file->IoControl(IOControl::CACHE_STATUS, status) >= 0)
    return true;
  else
    return false;
}

BitstreamStats CDVDInputStreamFile::GetBitstreamStats() const
{
  auto file = GetFile();
  if (!file)
    return m_stats; // dummy return. defined in CDVDInputStream

  if (file->GetBitstreamStats())
    return *file->GetBitstreamStats();
  else
    return m_stats;
}

// Use value returned by filesystem if is > 1
// otherwise defaults to 64K
int CDVDInputStreamFile::GetBlockSize()
{
  int chunk = 0;
  if (auto file = GetFile())
    chunk = file->GetChunkSize();

  return ((chunk > 1) ? chunk : 64 * 1024);
}

void CDVDInputStreamFile::SetReadRate(uint32_t rate)
{
  // Increase requested rate by 10%:
  uint32_t maxrate = static_cast<uint32_t>(1.1 * rate);

  auto file = GetFile();
  if (file && file->IoControl(IOControl::CACHE_SETRATE, &maxrate) >= 0)
    CLog::Log(LOGDEBUG,
              "CDVDInputStreamFile::SetReadRate - set cache throttle rate to {} bytes per second",
              maxrate);
}
