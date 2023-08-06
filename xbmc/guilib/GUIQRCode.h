/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIControl.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

#include <memory>
#include <string>

class CTexture;

namespace KODI
{
namespace GUILIB
{

class CGUIQRCode : public CGUIControl
{
public:
  CGUIQRCode(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIQRCode(const CGUIQRCode& other);
  ~CGUIQRCode() override;

  // Implementation of CGUIControl via CGUIImage
  CGUIQRCode* Clone() const override;
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;

  virtual void SetContent(const GUIINFO::CGUIInfoLabel& infoLabel);

private:
  void UpdateContent(const std::string& content);

  // GUI parameters
  GUIINFO::CGUIInfoLabel m_contentInfo;
  std::unique_ptr<CTexture> m_texture;
};

} /* namespace GUILIB */
} /* namespace KODI */
