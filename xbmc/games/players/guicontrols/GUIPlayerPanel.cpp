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

#include "GUIPlayerPanel.h"
#include "games/players/windows/GUIPlayerWindowDefines.h"

#include <tinyxml.h>

using namespace KODI;
using namespace GAME;

const CScroller CGUIPlayerPanel::m_scroller; //! @todo

CGUIPlayerPanel::CGUIPlayerPanel(int parentID, int controlID,
                                 float posX, float posY,
                                 float width, float height,
                                 const CLabelInfo &labelInfo,
                                 const CTextureInfo &textureFocus,
                                 const CTextureInfo &textureBackground) :
CGUIPanelContainer(parentID, controlID, posX, posY, width, height, HORIZONTAL, m_scroller, true),
  m_imgFocus(0, 0, 0, 0, textureFocus),
  m_imgBackground(0, 0, 0, 0, textureBackground),
  m_label(0, 0, 0, 0, labelInfo)
{
}

void CGUIPlayerPanel::OnSelect(unsigned int index)
{
  //! @todo
}
