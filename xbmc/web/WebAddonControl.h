/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "web/WebAddon.h"

#include <deque>
#include <memory>
#include <set>

namespace WEB
{

class CGUIWebAddonControl;
class CWebAddonControl;
typedef std::shared_ptr<CWebAddonControl> WebAddonControlPtr;

class CWebAddonControl : public std::enable_shared_from_this<CWebAddonControl>
{
public:
  CWebAddonControl(const std::string& id, const CWebAddonPtr& addon);
  ~CWebAddonControl();

  bool RegisterGUIControl(CGUIWebAddonControl* control);
  void UnregisterGUIControl(CGUIWebAddonControl* control);

  bool CreateWebControl(CGUIWebAddonControl* control, const std::string& startURL);
  void DestroyWebControl(CGUIWebAddonControl* control);

  CGUIWebAddonControl* GetFirstControl() { return m_firstGUIControl; }

  // -------------------------------------------------------------------------

  void Render();
  bool Dirty();
  bool OnInit();
  bool OnAction(int actionId, uint32_t buttoncode, wchar_t unicode, int& nextItem);
  bool OnMouseEvent(int id, double x, double y, double offsetX, double offsetY, int state);
  bool GetHistory(std::vector<std::string>& historyWebsiteNames, bool behindCurrent);
  void SearchText(const std::string& text, bool forward, bool matchCase, bool findNext);
  void StopSearch(bool clearSelection);
  void ScreenSizeChange(float x, float y, float width, float height, bool fullscreen);
  bool OpenWebsite(const std::string& url);
  void Reload();
  void StopLoad();
  void GoBack();
  void GoForward();
  void OpenOwnContextMenu();

  // -------------------------------------------------------------------------

  void SetControlReady(bool ready);
  void SetOpenedAddress(const std::string &address);
  void SetOpenedTitle(const std::string &title);
  void SetIconURL(const std::string &icon);
  void SetLoadingState(bool isLoading, bool canGoBack, bool canGoForward);
  void SetTooltip(const std::string &tooltip);
  void SetStatusMessage(const std::string &statusMessage);
  void RequestOpenSiteInNewTab(const std::string &url);

  // -------------------------------------------------------------------------

  bool IsLoading() const { return m_isLoading; }
  bool CanGoBack() const { return m_canGoBack; }
  bool CanGoForward() const { return m_canGoForward; }
  const std::string& OpenedAddress() const { return m_controlAddress; }
  const std::string& OpenedTitle() const { return m_controlTitle; }
  const std::string& IconURL() const { return m_controlIcon; }
  const std::string& StatusMessage() const { return m_statusMessage; }
  const std::string& Tooltip() const { return m_tooltyp; }

private:
  const std::string m_id;
  std::set<CGUIWebAddonControl*> m_guiControls;
  CGUIWebAddonControl* m_firstGUIControl;
  CGUIWebAddonControl* m_activeGUIControl;
  std::deque<CGUIWebAddonControl*> m_inactiveGUIControls;
  CWebAddonPtr m_addon;
  ADDON_HANDLE_STRUCT m_handle;

  bool m_isLoading;              /*!< Current browser work flag to inform web site load is active */
  bool m_canGoBack;              /*!< Flag where the message from add-on which allow go back becomes set */
  bool m_canGoForward;           /*!< Flag where the message from add-on which allow go forward becomes set */
  std::string m_controlAddress;  /*!< String with currently used address (URL) */
  std::string m_controlTitle;    /*!< The title of active web site */
  std::string m_statusMessage;   /*!< status message (e.g. selected URL) */
  std::string m_controlIcon;     /*!< Path to file in cache with web site icon */
  std::string m_tooltyp;         /*!< Tool tip of currently selected item web site */
};

} /* namespace WEB */
