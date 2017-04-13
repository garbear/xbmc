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

#include "PlatformDefs.h" // for __stat64
#include "interfaces/IAnnouncer.h"

#include <memory>
#include <string>
#include <sys/stat.h>

namespace KODI
{
namespace MEDIA
{
class CFileStore;
class CMediaStoreDatabase;
class CMediaStream;

class CMediaStore : public ANNOUNCEMENT::IAnnouncer
{
public:
  CMediaStore();
  virtual ~CMediaStore();

  void Initialize();
  void Deinitialize();

  int PinMedia(const std::string& path);
  bool IsPinned(unsigned int mediaId) const;
  bool IsPinned(const std::string& path) const;
  unsigned int GetMediaID(const std::string& path) const;
  void UnpinMedia(unsigned int mediaId);

  // File operations
  std::shared_ptr<CMediaStream> Open(unsigned int mediaId);
  bool Stat(unsigned int mediaId, struct __stat64* buffer);

  // implementation of IAnnouncer
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;

  // Utility functions
  static std::string BuildLocalPath(const std::string& mediaPath);

private:
  std::unique_ptr<CMediaStoreDatabase> m_database;
  std::unique_ptr<CFileStore> m_fileStore;
  bool m_bInitialized;
};

}
}
