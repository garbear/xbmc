/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIImage.h"

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIAvatarControl : public CGUIImage
{
public:
  CGUIAvatarControl(int parentID,
                    int controlID,
                    float posX,
                    float posY,
                    float width,
                    float height,
                    const CTextureInfo& texture);
  CGUIAvatarControl(const CGUIAvatarControl& from);

  ~CGUIAvatarControl() override = default;

  // Implementation of CGUIControl via CGUIImage
  CGUIAvatarControl* Clone() const override;
};
} // namespace GAME
} // namespace KODI
