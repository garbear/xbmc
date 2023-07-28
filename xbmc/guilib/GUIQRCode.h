/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIImage.h"

class CTextureInfo;

namespace KODI
{
namespace GUILIB
{

class CGUIQRCode : public CGUIImage
{
public:
  CGUIQRCode(int parentID,
             int controlID,
             float posX,
             float posY,
             float width,
             float height,
             const CTextureInfo& texture);
  CGUIQRCode(const CGUIQRCode& other);
  ~CGUIQRCode() override;

  // Implementation of CGUIControl via CGUIImage
  CGUIQRCode* Clone() const override;

private:
  void Initialize();
};

} /* namespace GUILIB */
} /* namespace KODI */
