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
#include "games/players/windows/IPlayerWindow.h"
#include "guilib/GUILabel.h"
#include "guilib/GUIListContainer.h"
#include "guilib/GUITexture.h"
#include "guilib/VisibleEffect.h" //! @todo

namespace KODI
{
namespace GAME
{
class CGUIPlayerList : public CGUIListContainer, public IPlayerList
{
public:
  CGUIPlayerList(int parentID,
                 int controlID,
                 float posX,
                 float posY,
                 float width,
                 float height,
                 ORIENTATION orientation,
                 const CScroller& scroller);
  CGUIPlayerList(const CGUIPlayerList& other);
  ~CGUIPlayerList() override = default;

  // implementation of CGUIControl via CGUIPanelContainer
  CGUIPlayerList* Clone() const override
  {
    return new CGUIPlayerList(*this);
  };
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;

  // implementation of IPlayerList
  void SetPlayerCount(unsigned int playerCount) override
  {
    m_playerCount = playerCount;
  }
  void OnSelect(unsigned int index) override;
  void SetGameClient(GameClientPtr gameClient) override
  {
    m_gameClient = gameClient;
  }

private:
  void InitializeBaseContainer();

  // Player properties
  unsigned int m_playerCount = 0;
  GameClientPtr m_gameClient;

  // State parameters
  ControllerVector m_players; //! @todo: Player array instead of controller array
};
} // namespace GAME
} // namespace KODI
