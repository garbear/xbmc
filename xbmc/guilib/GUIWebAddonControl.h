/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIControl.h"
#include "web/WebAddonControl.h"

namespace WEB
{

class CGUIWebAddonControl : public CGUIControl
{
public:
  CGUIWebAddonControl(int parentID, int controlID, float posX, float posY, float width, float height, const std::string& startAddress = "");
  CGUIWebAddonControl(const CGUIWebAddonControl& from);
  ~CGUIWebAddonControl() override;
  CGUIWebAddonControl *Clone() const override { return new CGUIWebAddonControl(*this); };

  void FreeResources(bool immediately = false) override;
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;
  void Render() override;
  void UpdateVisibility(const CGUIListItem* item) override;
  bool CanFocus() const override { return true; }
  bool CanFocusFromPoint(const CPoint &point) const override;
  void UpdateInfo(const CGUIListItem *item = nullptr) override;

  // Set functions for CGUIControlFactory
  void SetWebControlIdString(const std::string& id);
  void SetTransparent(bool transparent);
  void SetReloadButton(bool singleReload);
  void SetFullscreen(bool enable);

  // Interact functions between CGUIWebAddonControl and CWebAddonControl
  const WEB_ADDON_GUI_PROPS& GetProps() { return m_props; }
  void UpdateOpenedAddress(const std::string& address);
  void UpdateOpenedTitle(const std::string& title);
  void UpdateIconURL(const std::string& icon);
  void RequestOpenSiteInNewTab(const std::string& url);

  // GUI interface info functions (used from places where control is on window)
  const std::string& WebControlIdString() const { return m_idString; }
  bool IsLoading() const;
  bool CanGoBack() const;
  bool CanGoForward() const;
  const std::string& OpenedAddress() const;
  const std::string& OpenedTitle() const;
  const std::string& IconURL() const;
  const std::string& StatusMessage() const;
  const std::string& Tooltip() const;

  void SetLoadAddress(const std::string& loadAddress);
  bool GetHistory(std::vector<std::string>& historyWebsiteNames, bool behindCurrent);
  void SearchText(const std::string& text, bool forward, bool matchCase, bool findNext);
  void StopSearch(bool clearSelection);

  void SetWebAddonInvalid();

protected:
  EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event) override;

private:
  std::string GenerateAddress(const std::string& address);

  bool m_attemptedLoad;
  std::string m_idString;
  std::string m_startAddress;
  std::string m_loadAddress;
  WebAddonControlPtr m_addonControl;
  WEB_ADDON_GUI_PROPS m_props;
  bool m_transparentBackground;
  bool m_singleReload;

  // Full screen switch storage
  bool m_isFullScreen;
  float m_windowedXPos;
  float m_windowedYPos;
  float m_windowedWidth;
  float m_windowedHeight;
  float m_windowedAddonXPos;
  float m_windowedAddonYPos;
  float m_windowedAddonWidth;
  float m_windowedAddonHeight;
  float m_windowedSkinXPos;
  float m_windowedSkinYPos;
  float m_windowedSkinWidth;
  float m_windowedSkinHeight;
};

} /* namespace WEB */
