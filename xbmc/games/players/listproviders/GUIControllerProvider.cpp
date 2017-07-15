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

#include "GUIControllerProvider.h"
#include "GUIControllerItem.h"
#include "games/controllers/types/ControllerGrid.h"

using namespace KODI;
using namespace GAME;

CGUIControllerProvider::CGUIControllerProvider(int parentID) :
  IListProvider(parentID)
{
}

CGUIControllerProvider::~CGUIControllerProvider() = default;

bool CGUIControllerProvider::Update(bool forceRefresh)
{
  if (m_bUpdated)
  {
    m_bUpdated = false;
    return true;
  }

  return false;
}

bool CGUIControllerProvider::OnClick(const CGUIListItemPtr &item)
{
  CGUIListItem *listItem = static_cast<CGUIListItem*>(item.get());
  return false; //! @todo
}

void CGUIControllerProvider::SetGrid(const CControllerGrid &grid)
{
  m_items.clear();

  for (const auto &column : grid.Grid())
  {
    for (const auto &row : column.vertices)
    {
      if (row.bVisible)
        m_items.emplace_back(new CGUIControllerItem(row.controller));
      else
        m_items.emplace_back(new CGUIListItem);
    }
  }

  m_bUpdated = true;
}
