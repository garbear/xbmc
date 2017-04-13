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

#include "FileStore.h"
#include "MediaStream.h"

using namespace KODI;
using namespace MEDIA;

CFileStore::CFileStore()
{
}

CFileStore::~CFileStore()
{
  Deinitialize();
}

void CFileStore::Initialize()
{
  //! @todo Scan existing state from disk
}

void CFileStore::Deinitialize()
{
  m_streams.clear();
}

void CFileStore::PinMedia(unsigned int mediaId, const std::string& path)
{
  Open(mediaId, path);
}

void CFileStore::UnpinMedia(unsigned int mediaId)
{
  Close(mediaId);
}

std::shared_ptr<CMediaStream> CFileStore::Open(unsigned int mediaId, const std::string& path)
{
  std::shared_ptr<CMediaStream> stream = GetStream(mediaId);

  if (!stream)
  {
    stream.reset(new CMediaStream(path));
    if (!stream->Open())
      stream.reset();
    else
      m_streams[mediaId] = stream;
  }

  return stream;
}

void CFileStore::Close(unsigned int mediaId)
{
  auto it = m_streams.find(mediaId);
  if (it != m_streams.end())
  {
    it->second->Close();
    m_streams.erase(it);
  }
}

std::shared_ptr<CMediaStream> CFileStore::GetStream(unsigned int mediaId) const
{
  std::shared_ptr<CMediaStream> stream;

  auto it = m_streams.find(mediaId);
  if (it != m_streams.end())
    stream = it->second;

  return stream;
}
