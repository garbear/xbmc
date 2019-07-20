/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"

#include <memory>

namespace WEB
{

class CFavouriteEntry;
typedef std::shared_ptr<CFavouriteEntry> FavouriteEntryPtr;

class CFavouriteEntry
{
public:
  CFavouriteEntry(const std::string& name, const std::string& url, const std::string& icon, const CDateTime& dateTimeAdded);

  const std::string& GetName() { return m_name; }
  const std::string& GetURL() { return m_url; }
  const std::string& GetIcon() { return m_icon; }
  const CDateTime& GetDateTimeAdded() { return m_dateTimeAdded; }

private:
  std::string m_name;
  std::string m_url;
  std::string m_icon;
  CDateTime m_dateTimeAdded;
};

class CWebFavourites
{
public:
  static void AddToFavourites(const std::string& name, const std::string& url, const std::string& icon);
};

}
