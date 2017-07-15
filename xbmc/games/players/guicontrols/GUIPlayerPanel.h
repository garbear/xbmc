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
#include "guilib/GUILabel.h"
#include "guilib/GUIPanelContainer.h"
#include "guilib/GUITexture.h"
#include "guilib/VisibleEffect.h" //! @todo

namespace KODI
{
namespace GAME
{
  class CGUIPlayerPanel : public CGUIPanelContainer,
                          public IPlayerPanel
  {
  public:
    CGUIPlayerPanel(int parentID, int controlID, float posX, float posY, float width, float height,
                    const CLabelInfo &labelInfo, const CTextureInfo &textureFocus, const CTextureInfo &textureBackground);
    ~CGUIPlayerPanel() override = default;

    // implementation of CGUIControl via CGUIPanelContainer
    CGUIPlayerPanel *Clone() const override { return new CGUIPlayerPanel(*this); };

    // implementation of IPlayerPanel
    void SetAvatarSize(unsigned int sizePx) override { m_avatarSize = sizePx; }
    void SetPlayerCount(unsigned int playerCount) override { m_playerCount = playerCount; }
    void OnSelect(unsigned int index) override;

  private:
    // GUI properties
    CGUITexture m_imgFocus;
    CGUITexture m_imgBackground;
    CGUILabel m_label;

    // Player properties
    unsigned int m_avatarSize = 0;
    unsigned int m_playerCount = 0;

    void *m_focusedLayout = nullptr; //! @todo


    static const CScroller m_scroller; //! @todo
  };
}
}
