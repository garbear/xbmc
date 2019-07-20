/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Favourites.h"
#include "WebDatabase.h"

#include "XBDateTime.h"
#include "utils/log.h"

namespace WEB
{

CFavouriteEntry::CFavouriteEntry(const std::string& name, const std::string& url, const std::string& icon, const CDateTime& dateTimeAdded)
  : m_name(name),
    m_url(url),
    m_icon(icon),
    m_dateTimeAdded(dateTimeAdded)
{

}

void CWebFavourites::AddToFavourites(const std::string& name, const std::string& url, const std::string& icon)
{
  CWebDatabase database;
  if (!database.Open())
  {
    CLog::Log(LOGERROR, "CWebFavourites::%s: Failed to opened web database");
    return;
  }

  FavouriteEntryPtr entry = std::make_shared<CFavouriteEntry>(name, url, icon, CDateTime::GetCurrentDateTime());
  database.AddFavourite(entry);
}

}
