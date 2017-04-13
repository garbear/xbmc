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

#include <set>
#include <string>

namespace KODI
{
namespace MEDIA
{

class CMediaFile;

class CMediaStoreDatabase
{
public:
  CMediaStoreDatabase();
  ~CMediaStoreDatabase();

  void Initialize();
  void Deinitialize();

  int PinMedia(const std::string& path);
  bool IsPinned(unsigned int mediaId) const;
  bool IsPinned(const std::string& path) const;
  std::string GetPath(unsigned int mediaId) const;
  unsigned int GetMediaID(const std::string& path) const;
  void UnpinMedia(unsigned int mediaId);
  void UnpinMedia(const std::string& path);

  void Load();
  void Save();

private:
  struct Media
  {
    Media(std::string path) :
      mediaId(0),
      path(std::move(path))
    {
    }

    Media(unsigned int mediaId) :
      mediaId(mediaId)
    {
    }

    Media(unsigned int mediaId, std::string path) :
      mediaId(mediaId),
      path(std::move(path))
    {
    }

    unsigned int mediaId;
    std::string path;
  };

  struct MediaIDLess
  {
    bool operator()(const Media& lhs, const Media& rhs) const
    {
      return lhs.mediaId < rhs.mediaId;
    }
  };

  struct MediaPathLess
  {
    bool operator()(const Media& lhs, const Media& rhs) const
    {
      return lhs.path < rhs.path;
    }
  };

  std::set<Media, MediaIDLess> m_pinnedById;
  std::set<Media, MediaPathLess> m_pinnedByPath;

  unsigned int m_maxMediaId;
};

}
}
