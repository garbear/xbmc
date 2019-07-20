/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "WebDirectory.h"
#include "ServiceBroker.h"

#include "File.h"
#include "Directory.h"
#include "Util.h"
//#include "profiles/ProfilesManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include "web/WebManager.h"

namespace XFILE
{

bool CWebDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string base(url.Get());
  URIUtils::RemoveSlashAtEnd(base);

  std::string fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);
  CLog::Log(LOGDEBUG, "CWebDirectory::GetDirectory(%s)", base.c_str());
  items.SetCacheToDisc(CFileItemList::CACHE_NEVER);

  if (StringUtils::StartsWith(fileName, "sites/favourites"))
  {
    return CServiceBroker::GetWEBManager().GetFavourites(url.Get(), items);
  }
  else if (StringUtils::StartsWith(fileName, "sites/running"))
  {
    return CServiceBroker::GetWEBManager().GetRunningWebsites(url.Get(), items);
  }

  return false;
}

bool CWebDirectory::Exists(const CURL& url)
{
  if (!CServiceBroker::GetWEBManager().IsStarted())
    return false;

  return url.IsProtocol("web");
}

} // namespace XFILE
