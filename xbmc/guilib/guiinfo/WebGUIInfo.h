/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CWebGUIInfo : public CGUIInfoProvider
{
public:
  CWebGUIInfo() = default;
  ~CWebGUIInfo() override = default;

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem* item) override;
  bool GetLabel(std::string& value, const CFileItem* item, int contextWindow, const CGUIInfo& info, std::string* fallback) const override;
  bool GetInt(int& value, const CGUIListItem* item, int contextWindow, const CGUIInfo& info) const override;
  bool GetBool(bool& value, const CGUIListItem* item, int contextWindow, const CGUIInfo& info) const override;

private:
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
