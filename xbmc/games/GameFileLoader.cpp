/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GameFileLoader.h"
#include "GameClient.h"
#include "games/libretro/libretro_wrapped.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "URL.h"

#include <limits>

using namespace ADDON;
using namespace GAME_INFO;
using namespace GAMES;
using namespace XFILE;


/* static */
bool CGameFileLoader::CanOpen(const CFileItem &file, const GameClientConfig &config, bool useStrategies /* = false */)
{
  // Check gameclient
  if (file.HasProperty("gameclient") && file.GetProperty("gameclient").asString() != config.id)
    return false;

  if (config.extensions.empty() && config.platforms.empty())
    return true; // Client provided us with *no* useful information. Be optimistic.

  // Check platform
  if (!config.platforms.empty() && file.GetGameInfoTag())
  {
    GamePlatform id = CGameInfoTagLoader::GetPlatformByName(file.GetGameInfoTag()->GetPlatform()).id;
    if (id != PLATFORM_UNKNOWN)
      if (std::find(config.platforms.begin(), config.platforms.end(), id) == config.platforms.end())
        return false;
  }

  // If we don't do thorough screening, at least check inside the zip for valid files
  if (!useStrategies)
  {
    // Get the file extension (we want .zip if the file is a top-level zip directory)
    CStdString ext(URIUtils::GetExtension(file.GetAsUrl().GetFileNameWithoutPath()));
    if (ext == ".zip")
    {
      // If .zip is not valid, see if there is a file inside that is
      CStdString path2;
      return IsExtensionValid(ext, config.extensions) || GetEffectiveRomPath(file.GetPath(), config.extensions, path2);
    }
    // If the game client lists extensions, check those as well
    return config.extensions.empty() || config.extensions.find(ext) != config.extensions.end();
  }
  else
  {
    CGameFileLoaderUseHD        hd;
    CGameFileLoaderUseParentZip outerzip;
    CGameFileLoaderUseVFS       vfs;
    CGameFileLoaderEnterZip     innerzip;

    CGameFileLoader *strategy[4] = { &hd, &outerzip, &vfs, &innerzip };

    for (unsigned int i = 0; i < sizeof(strategy) / sizeof(strategy[0]); i++)
      if (strategy[i]->CanLoad(config, file))
        return true;
    return false;
  }
}

/* static */
bool CGameFileLoader::GetEffectiveRomPath(const CStdString &zipPath, const std::set<CStdString> &validExts, CStdString &effectivePath)
{
  // Default case: effective zip file is the zip file itself
  effectivePath = zipPath;

  // If it's not a zip file, we can't open and explore...
  if (!URIUtils::GetExtension(zipPath).Equals(".zip"))
    return false;

  // Enumerate the zip directory, looking for valid extensions
  CStdString strUrl;
  URIUtils::CreateArchivePath(strUrl, "zip", zipPath, "");

  CStdString strValidExts;
  for (std::set<CStdString>::const_iterator it = validExts.begin(); it != validExts.end(); it++)
    strValidExts += *it + "|";

  CFileItemList itemList;
  if (CDirectory::GetDirectory(strUrl, itemList, strValidExts, DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO) && itemList.Size())
  {
    // Use the first file discovered
    effectivePath = itemList[0]->GetPath();
    return true;
  }
  return false;
}

/* static */
bool CGameFileLoader::IsExtensionValid(const CStdString &ext, const std::set<CStdString> &setExts)
{
  if (setExts.empty())
    return true; // Be optimistic :)
  if (ext.empty())
    return false;
  CStdString ext2(ext);
  ext2.ToLower();
  if (ext2.at(0) != '.')
    ext2 = "." + ext2;
  return setExts.find(ext2) != setExts.end();
}

bool CGameFileLoader::GetGameInfo(LIBRETRO::retro_game_info &info) const
{
  // Always return path. If info.data is set, then info.path must be set back to NULL
  info.path = m_path.c_str();
  info.data = NULL;
  info.size = 0;
  info.meta = NULL;

  if (!m_useVfs)
  {
    CLog::Log(LOGDEBUG, "GameClient: Strategy is valid, client is loading file %s", info.path);
  }
  else
  {
    uint8_t *data;
    int64_t length;

    // Load the file from the vfs
    CFile vfsFile;
    if (!vfsFile.Open(m_path))
    {
      CLog::Log(LOGERROR, "GameClient::CStrategyUseVFS: XBMC cannot open file");
      return false; // XBMC can't load it, don't expect the game client to
    }

    length = vfsFile.GetLength();

    // Check for file size overflow (libretro accepts files <= size_t max)
    if (length <= 0 || (uint64_t)length > (uint64_t)std::numeric_limits<size_t>::max())
    {
      CLog::Log(LOGERROR, "GameClient: Invalid file size: %"PRId64" bytes", length);
      return false;
    }

    data = new uint8_t[(size_t)length];

    // Verify the allocation and read in the data
    if (!(data && vfsFile.Read(data, length) == length))
    {
      CLog::Log(LOGERROR, "GameClient: XBMC failed to read game data");
      delete[] data;
      return false;
    }

    info.data = data;
    info.size = (size_t)length;
    CLog::Log(LOGDEBUG, "GameClient: Strategy is valid, client is loading file from VFS (filesize: %lu KB)", info.size);
  }
  return true;
}

bool CGameFileLoaderUseHD::CanLoad(const GameClientConfig &gc, const CFileItem& file)
{
  CLog::Log(LOGDEBUG, "GameClient::CStrategyUseHD: Testing if we can load game from hard drive");

  // Make sure the file is local
  if (!file.GetAsUrl().GetProtocol().empty())
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseHD: File is not local (or is inside an archive)");
    return false;
  }

  // Make sure the extension is valid
  if (!IsExtensionValid(URIUtils::GetExtension(file.GetPath()), gc.extensions))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseHD: Extension %s is not valid", URIUtils::GetExtension(file.GetPath()).c_str());
    return false;
  }

  m_path = file.GetPath();
  m_useVfs = false;
  return true;
}

bool CGameFileLoaderUseVFS::CanLoad(const GameClientConfig &gc, const CFileItem& file)
{
  CLog::Log(LOGDEBUG, "GameClient::CStrategyUseVFS: Testing if we can load game from VFS");

  // Obvious check
  if (!gc.bAllowVFS)
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseVFS: Game client does not allow VFS");
    return false;
  }

  // Make sure the extension is valid
  CStdString ext = URIUtils::GetExtension(file.GetPath());
  if (!IsExtensionValid(ext, gc.extensions))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseVFS: Extension %s is not valid", ext.c_str());
    return false;
  }

  m_path = file.GetPath();
  m_useVfs = true;
  return true;
}

bool CGameFileLoaderUseParentZip::CanLoad(const GameClientConfig &gc, const CFileItem& file)
{
  CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Testing if the game is in a zip");

  // Can't use parent zip if file isn't a child file of a .zip folder
  if (!URIUtils::IsInZIP(file.GetPath()))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Game is not in a zip file");
    return false;
  }

  if (!IsExtensionValid(".zip", gc.extensions))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: This game client does not support zip files");
    return false;
  }

  // Make sure we're in the root folder of the zip (no parent folder)
  CURL parentURL(URIUtils::GetParentPath(file.GetPath()));
  if (!parentURL.GetFileName().empty())
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Game is not in the root folder of the zip");
    return false;
  }

  // Make sure the container zip is on the local hard disk (or not inside another zip)
  if (!CURL(parentURL.GetHostName()).GetProtocol().empty())
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Zip file is not on the local hard disk");
    return false;
  }

  // Make sure the extension is valid
  if (!IsExtensionValid(URIUtils::GetExtension(file.GetPath()), gc.extensions))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Extension %s is not valid", URIUtils::GetExtension(file.GetPath()).c_str());
    return false;
  }

  // Found our file
  m_path = parentURL.GetHostName();
  m_useVfs = false;
  return true;
}

bool CGameFileLoaderEnterZip::CanLoad(const GameClientConfig &gc, const CFileItem& file)
{
  CLog::Log(LOGDEBUG, "GameClient::CStrategyEnterZip: Testing if the file is a zip containing a game");

  // Must be a zip file, clearly
  if (!URIUtils::GetExtension(file.GetPath()).Equals(".zip"))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyEnterZip: File is not a zip");
    return false;
  }

  // Must support loading from the vfs
  if (!gc.bAllowVFS)
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyEnterZip: Game client does not allow VFS");
    return false;
  }

  // Look for an internal file. This will screen against valid extensions.
  CStdString internalFile;
  if (!GetEffectiveRomPath(file.GetPath(), gc.extensions, internalFile))
  {
    CLog::Log(LOGINFO, "GameClient::CStrategyEnterZip: Zip does not contain a file with a valid extension");
    return false;
  }

  m_path = internalFile;
  m_useVfs = true;
  return true;
}
