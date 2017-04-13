/*
 *      Copyright (C) 2017 Unique Digital Ltd.
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "IFile.h"

#include <memory>
#include <stdint.h>

namespace KODI
{
namespace MEDIA
{
  class CMediaStream;
}
}

namespace XFILE
{

class CFile;

class CMediaFile : public IFile
{
public:
  CMediaFile();
  virtual ~CMediaFile() { Close(); }

  // implementation of IFile
  virtual bool Open(const CURL& url) override;
  //virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false) override { return false; }
  //virtual bool ReOpen(const CURL& url) override { return false; }
  virtual bool Exists(const CURL& url) override;
  virtual int Stat(const CURL& url, struct __stat64* buffer) override;
  virtual int Stat(struct __stat64* buffer) override;
  virtual ssize_t Read(void* bufPtr, size_t bufSize) override;
  //virtual ssize_t Write(const void* bufPtr, size_t bufSize) override { return -1; }
  virtual bool ReadString(char *szLine, int iLineLength) override;
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  virtual void Close() override;
  virtual int64_t GetPosition() override;
  virtual int64_t GetLength() override;
  //virtual void Flush() override { }
  //virtual int Truncate(int64_t size) override { return -1; }
  virtual int GetChunkSize() override;
  virtual double GetDownloadSpeed() override;
  //virtual bool Delete(const CURL& url) override { return false; }
  //virtual bool Rename(const CURL& url, const CURL& urlnew) override { return false; }
  //virtual bool SetHidden(const CURL& url, bool hidden) override { return false; }
  virtual int IoControl(EIoControl request, void* param) override;
  virtual std::string GetContent();
  //virtual std::string GetContentCharset() override;

  static unsigned int GetMediaID(const std::string& mediaStorePath);
  static std::string GetMediaStorePath(unsigned int mediaId);

private:
  std::shared_ptr<KODI::MEDIA::CMediaStream> m_mediaStream;
  int64_t m_position;
};

}
