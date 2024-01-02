/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAvatarControl.h"

using namespace KODI;
using namespace GAME;

CGUIAvatarControl::CGUIAvatarControl(int parentID,
                                     int controlID,
                                     float posX,
                                     float posY,
                                     float width,
                                     float height,
                                     const CTextureInfo& texture)
  : CGUIImage(parentID, controlID, posX, posY, width, height, texture)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_AVATAR;
}

CGUIAvatarControl::CGUIAvatarControl(const CGUIAvatarControl& from) : CGUIImage(from)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_AVATAR;
}

CGUIAvatarControl* CGUIAvatarControl::Clone(void) const
{
  return new CGUIAvatarControl(*this);
}
