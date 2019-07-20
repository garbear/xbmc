/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

namespace WEB
{

class CGUIWebAddonControl;

class CGUIWindowWebBrowserFullScreen : public CGUIWindow
{
public:
  CGUIWindowWebBrowserFullScreen(void);

  void OnInitWindow(void) override;
  void OnDeinitWindow(int nextWindowID) override;
  bool OnAction(const CAction &action) override;

private:
  bool m_useRunningControl;
  CGUIWebAddonControl* m_webControl;
};

} /* namespace WEB */
