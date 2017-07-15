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

#include "GUIControllerPanel.h"
#include "games/controllers/guicontrols/GUIGameController.h"
#include "games/controllers/types/ControllerGrid.h"
#include "games/players/listproviders/GUIControllerProvider.h"
#include "games/players/windows/GUIPlayerWindowDefines.h"
#include "guilib/GUIListItemLayout.h"

#include <tinyxml.h>

using namespace KODI;
using namespace GAME;

const CScroller CGUIControllerPanel::m_scroller; //! @todo

CGUIControllerPanel::CGUIControllerPanel(int parentID, int controlID,
                                         float posX, float posY,
                                         float width, float height,
                                         const CTextureInfo &textureFocus,
                                         const CTextureInfo &textureBackground) :
  CGUIPanelContainer(parentID, controlID, posX, posY, width, height, HORIZONTAL, m_scroller, true),
  m_imgFocus(0, 0, 0, 0, textureFocus),
  m_imgBackground(0, 0, 0, 0, textureBackground)
{
  InitializeBaseContainer();
}

CGUIControllerPanel::CGUIControllerPanel(const CGUIControllerPanel &other) :
  CGUIPanelContainer(other),
  m_imgFocus(other.m_imgFocus),
  m_imgBackground(other.m_imgBackground),
  m_alignment(other.m_alignment),
  m_grid(new CControllerGrid(*other.m_grid))
{
  InitializeBaseContainer();
}

void CGUIControllerPanel::InitializeBaseContainer()
{
  // Initialize CGUIBaseContainer
  m_listProvider = new CGUIControllerProvider(GetParentID());
}

CGUIControllerPanel::~CGUIControllerPanel()
{
}

void CGUIControllerPanel::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CGUIPanelContainer::Process(currentTime, dirtyregions);

  //! @todo
}

void CGUIControllerPanel::Render()
{
  CGUIPanelContainer::Render();

  //! @todo
}

void CGUIControllerPanel::SetGrid(const CControllerGrid &grid)
{
  CGUIControllerProvider *controllerProvider = dynamic_cast<CGUIControllerProvider*>(m_listProvider);
  if (controllerProvider != nullptr)
    controllerProvider->SetGrid(grid);

  m_grid.reset(new CControllerGrid(grid));

  // Update layouts
  if (m_layout)
  {
    m_layout->SetWidth(GetItemSize());
    m_layout->SetHeight(GetItemSize());
  }
  if (m_focusedLayout)
  {
    m_focusedLayout->SetWidth(GetItemSize());
    m_focusedLayout->SetHeight(GetItemSize());
  }
}

unsigned int CGUIControllerPanel::GetItemSize() const
{
  if (m_grid)
  {
    const unsigned int height = m_grid->Height();
    if (height > 0)
      return m_height / height;
  }

  return 0;
}

void CGUIControllerPanel::OnSelect(unsigned int column, unsigned int row)
{
  //! @todo
}

ControllerVector CGUIControllerPanel::GetControllers(unsigned int col, unsigned int row)
{
  ControllerVector controllers;

  //! @todo

  return controllers;
}
