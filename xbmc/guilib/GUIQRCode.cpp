/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIQRCode.h"

#include "GUITexture.h"

using namespace KODI;
using namespace GUILIB;

CGUIQRCode::CGUIQRCode(int parentID,
                       int controlID,
                       float posX,
                       float posY,
                       float width,
                       float height,
                       const CTextureInfo& texture)
  : CGUIImage(parentID, controlID, posX, posY, width, height, texture)
{
  Initialize();
}

CGUIQRCode::CGUIQRCode(const CGUIQRCode& other) : CGUIImage(other)
{
  Initialize();
}

CGUIQRCode::~CGUIQRCode() = default;

void CGUIQRCode::Initialize()
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_QRCODE;

  // Initialize CGUIImage
  SetAspectRatio(CAspectRatio{CAspectRatio::AR_KEEP});
  SetScalingMethod(TEXTURE_SCALING::NEAREST);
}

CGUIQRCode* CGUIQRCode::Clone() const
{
  return new CGUIQRCode(*this);
}
