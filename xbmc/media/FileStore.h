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

#include <map>
#include <memory>

namespace KODI
{
namespace MEDIA
{
class CMediaStream;

class CFileStore
{
public:
  CFileStore();
  ~CFileStore();
  
  void Initialize();
  void Deinitialize();

  void PinMedia(unsigned int mediaId, const std::string& path);
  void UnpinMedia(unsigned int mediaId);

  std::shared_ptr<CMediaStream> Open(unsigned int mediaId, const std::string& path);
  void Close(unsigned int mediaId);

  std::shared_ptr<CMediaStream> GetStream(unsigned int mediaId) const;

private:
  std::map<unsigned int, std::shared_ptr<CMediaStream>> m_streams;
};

}
}
