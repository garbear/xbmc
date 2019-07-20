/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/Database.h"
#include "web/Favourites.h"

namespace WEB
{

class CWebDatabase : public CDatabase
{
public:
  CWebDatabase();
  ~CWebDatabase() override;
  bool Open() override;

  bool AddFavourite(FavouriteEntryPtr entry);
  bool RemoveFavourite(const std::string& name, const std::string& url);
  inline bool RemoveFavourite(FavouriteEntryPtr entry) { return RemoveFavourite(entry->GetName(), entry->GetURL()); }
  bool GetFavourites(std::vector<FavouriteEntryPtr>& entries);
  bool ClearFavourites();

protected:
  void CreateTables() override;
  void CreateAnalytics() override;
  void UpdateTables(int version) override;
  int GetSchemaVersion() const override { return 1; }
  const char *GetBaseDBName() const override { return "WEB"; }
};

} /* namespace WEB */
