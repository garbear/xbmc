/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windows/GUIMediaWindow.h"
#include "settings/lib/ISettingCallback.h"
#include "utils/JobManager.h"
#include "utils/Observer.h"
#include "web/WebAddon.h"

namespace WEB
{

class CGUIWebAddonControl;

class CGUIWindowWebBrowser : public CGUIWindow
{
public:
  CGUIWindowWebBrowser(void);
  ~CGUIWindowWebBrowser(void) override;

  void OnInitWindow(void) override;
  void OnDeinitWindow(int nextWindowID) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  bool OnBack(int actionID) override;
  void SetInvalid() override;
  void OnWindowLoaded(void) override;
  void OnWindowUnload(void) override;
  bool HasWebControl() const override { return true; }

  std::string SelectedAddress();
  const std::string &OpenedAddress();
  const std::string &OpenedTitle();
  const std::string &Tooltip();
  const std::string &StatusMessage();
  const std::string &IconURL();
  bool IsLoading();
  bool CanGoBack();
  bool CanGoForward();

  CGUIWebAddonControl* GetRunningControl();
  bool ControlDestroyAll();

private:
  bool ControlCreate(const std::string& url = "");
  bool ControlDestroy(int itemNo, bool destroyAllOthers);
  bool SelectTab(int tab);

  CGUIWebAddonControl* m_originalWebControl;
  CGUIWebAddonControl* m_activeWebControl;
  std::map<int, CGUIWebAddonControl*> m_controls;
  std::unique_ptr<CFileItemList> m_vecItems;
  CGUIViewControl m_viewControl;
  int m_selectedItem;
  unsigned int m_nextBrowserId;
  std::string m_lastSearchLabel;
};

} /* namespace WEB */
