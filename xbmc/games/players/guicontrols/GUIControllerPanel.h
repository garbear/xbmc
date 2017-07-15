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

#include "games/players/windows/IPlayerWindow.h"
#include "guilib/GUIPanelContainer.h"
#include "guilib/GUITexture.h"
#include "guilib/VisibleEffect.h" //! @todo

namespace KODI
{
namespace GAME
{
  class CControllerGrid;
  class CGUIGameController;

  class CGUIControllerPanel : public CGUIPanelContainer,
                              public IControllerPanel
  {
  public:
    CGUIControllerPanel(int parentID, int controlID, float posX, float posY, float width, float height,
                        const CTextureInfo &textureFocus, const CTextureInfo &textureBackground);
    CGUIControllerPanel(const CGUIControllerPanel &other);
    ~CGUIControllerPanel() override;

    // implementation of CGUIControl via CGUIPanelContainer
    CGUIControllerPanel *Clone() const override { return new CGUIControllerPanel(*this); };
    void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
    void Render() override;

    // implementation of IControllerPanel
    void SetGrid(const CControllerGrid &grid) override;
    unsigned int GetItemSize() const override;
    void OnSelect(unsigned int column, unsigned int row) override;
    ControllerVector GetControllers(unsigned int column, unsigned int row) override;

  private:
    void InitializeBaseContainer();

    // GUI properties
    CGUITexture m_imgFocus;
    CGUITexture m_imgBackground;
    int m_alignment = 1; //! @todo

    // Game properties
    std::unique_ptr<CControllerGrid> m_grid;

    static const CScroller m_scroller; //! @todo
  };
}
}
