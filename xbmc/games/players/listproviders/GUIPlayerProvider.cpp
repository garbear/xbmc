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

#include "GUIPlayerProvider.h"
#include "GUIPlayerItem.h"
#include "games/controllers/Controller.h"

using namespace KODI;
using namespace GAME;

CGUIPlayerProvider::CGUIPlayerProvider(int parentID)
    : IListProvider(parentID)
{
}

CGUIPlayerProvider::~CGUIPlayerProvider() = default;

bool CGUIPlayerProvider::Update(bool forceRefresh)
{
  if (m_bUpdated)
  {
    m_bUpdated = false;
    return true;
  }

  return false;
}

bool CGUIPlayerProvider::OnClick(const CGUIListItemPtr& item)
{
  CGUIListItem* listItem = static_cast<CGUIListItem*>(item.get());
  return false; //! @todo
}

bool CGUIPlayerProvider::OnInfo(const CGUIListItemPtr& item)
{
  //! @todo
  return false;
}

bool CGUIPlayerProvider::OnContextMenu(const CGUIListItemPtr& item)
{
  //! @todo
  return false;
}

void CGUIPlayerProvider::SetPlayers(const ControllerVector& players)
{
  m_items.clear();

  for (const auto& player : players)
    m_items.emplace_back(new CGUIPlayerItem(player));

  // Update state
  m_bUpdated = true;

  return;
}
