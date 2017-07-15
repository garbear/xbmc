/*
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

#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "guilib/GUIListItem.h"
#include "listproviders/IListProvider.h"

#include <vector>

namespace KODI
{
namespace GAME
{
class CGUIPlayerProvider : public IListProvider
{
public:
  CGUIPlayerProvider(int parentID);
  ~CGUIPlayerProvider() override;

  // implementation of IListProvider
  bool Update(bool forceRefresh) override;
  void Fetch(std::vector<CGUIListItemPtr>& items) override
  {
    items = m_items;
  }
  bool OnClick(const CGUIListItemPtr& item) override;
  bool OnInfo(const CGUIListItemPtr& item) override;
  bool OnContextMenu(const CGUIListItemPtr& item) override;

  // Player interface
  void SetPlayers(const ControllerVector& players);

private:
  // GUI parameters
  std::vector<CGUIListItemPtr> m_items;

  // State parameters
  bool m_bUpdated = false;
};
} // namespace GAME
} // namespace KODI
