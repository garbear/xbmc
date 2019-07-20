/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/WebGUIInfo.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

#include "web/WebManager.h"
#include "web/windows/GUIWindowWebBrowser.h"

using namespace KODI::GUILIB::GUIINFO;

bool CWebGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CWebGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo& info, std::string* fallback) const
{
  if (info.m_info >= WEB_CONTROL_START && info.m_info <= WEB_CONTROL_END)
  {
    WEB::CGUIWindowWebBrowser* window = GetWebBrowserWindow(contextWindow);
    if (window)
    {
      switch (info.m_info)
      {
        case WEB_CONTROL_OPENED_ADDRESS:
          value = window->OpenedAddress();
          return true;
        case WEB_CONTROL_OPENED_TITLE:
          value = window->OpenedTitle();
          return true;
        case WEB_CONTROL_TOOLTIP:
          value = window->Tooltip();
          return true;
        case WEB_CONTROL_STATUS_MESSAGE:
          value = window->StatusMessage();
          return true;
      }
    }
  }

  return false;
}

bool CWebGUIInfo::GetInt(int& value, const CGUIListItem* item, int contextWindow, const CGUIInfo& info) const
{
  return false;
}

bool CWebGUIInfo::GetBool(bool& value, const CGUIListItem* item, int contextWindow, const CGUIInfo& info) const
{
  if (info.m_info >= WEB_CONTROL_START && info.m_info <= WEB_CONTROL_END)
  {
    WEB::CGUIWindowWebBrowser* window = GetWebBrowserWindow(contextWindow); // true for has web control
    if (window)
    {
      switch (info.m_info)
      {
        case WEB_CONTROL_IS_LOADING:
          value = window->IsLoading();
          return true;
        case WEB_CONTROL_CAN_GO_BACK:
          value = window->CanGoBack();
          return true;
        case WEB_CONTROL_CAN_GO_FORWARD:
          value = window->CanGoForward();
          return true;
      }
    }
  }

  switch (info.m_info)
  {
    case WEB_HAS_ADDONS:
      value = CServiceBroker::GetWEBManager().HasCreatedAddons();
      return true;
    case WEB_HAS_BROWSER:
      value = CServiceBroker::GetWEBManager().HasBrowser();
      return true;
  }

  return false;
}
