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

#include "games/controllers/ControllerTypes.h"
#include "games/GameTypes.h"
#include "guilib/GUIListItem.h"
#include "listproviders/IListProvider.h"

#include <vector>

namespace KODI
{
  namespace GAME
  {
    class CControllerGrid;

    class CGUIControllerProvider : public IListProvider
    {
    public:
      CGUIControllerProvider(int parentID);
      ~CGUIControllerProvider() override;

      // implementation of IListProvider
      bool Update(bool forceRefresh) override;
      void Fetch(std::vector<CGUIListItemPtr> &items) override { items = m_items; }
      bool OnClick(const CGUIListItemPtr &item) override;
      bool OnInfo(const CGUIListItemPtr &item) override { return false; }
      bool OnContextMenu(const CGUIListItemPtr &item) override { return false; }
      void SetDefaultItem(int item, bool always) override { } //! @todo
      int GetDefaultItem() const override { return -1; } //! @todo
      bool AlwaysFocusDefaultItem() const override { return false; } //! @todo

      // Controller interface
      void SetGrid(const CControllerGrid &grid);

    private:
      std::vector<CGUIListItemPtr> m_items;
      bool m_bUpdated = false;
    };
  }
}
