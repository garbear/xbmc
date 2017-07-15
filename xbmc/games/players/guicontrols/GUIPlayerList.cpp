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

#include "GUIPlayerList.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/controllers/Controller.h"
#include "games/players/listproviders/GUIPlayerProvider.h"

#include <array>
#include <tinyxml.h>

using namespace KODI;
using namespace GAME;

CGUIPlayerList::CGUIPlayerList(int parentID,
                               int controlID,
                               float posX,
                               float posY,
                               float width,
                               float height,
                               ORIENTATION orientation,
                               const CScroller& scroller)
    : CGUIListContainer(parentID, controlID, posX, posY, width, height, orientation, scroller, true)
{
  InitializeBaseContainer();
}

CGUIPlayerList::CGUIPlayerList(const CGUIPlayerList& other)
    : CGUIListContainer(other)
{
  InitializeBaseContainer();
}

void CGUIPlayerList::InitializeBaseContainer()
{
  // Initialize CGUIBaseContainer
  m_listProvider = new CGUIPlayerProvider(GetParentID());
}

void CGUIPlayerList::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  if (m_gameClient)
  {
    ControllerVector players = m_gameClient->Input().GetPlayers();
    if (players != m_players)
    {
      m_players = players;

      auto playerProvider = static_cast<CGUIPlayerProvider*>(m_listProvider);
      playerProvider->SetPlayers(players);
    }
  }

  CGUIListContainer::Process(currentTime, dirtyregions);
}

void CGUIPlayerList::OnSelect(unsigned int index)
{
  //! @todo
}
