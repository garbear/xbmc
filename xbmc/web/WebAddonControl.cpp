/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebAddonControl.h"
#include "WebManager.h"

#include "ServiceBroker.h"
#include "guilib/GUIWebAddonControl.h"
#include "utils/log.h"

using namespace WEB;

CWebAddonControl::CWebAddonControl(const std::string& id, const CWebAddonPtr& addon)
  : m_id(id),
    m_firstGUIControl(nullptr),
    m_activeGUIControl(nullptr),
    m_addon(addon)
{
  m_handle.dataIdentifier = -1;
  m_handle.callerAddress = nullptr;
  m_handle.dataAddress = nullptr;
}

CWebAddonControl::~CWebAddonControl()
{
  m_addon->DestroyControl(&m_handle, true);
  CServiceBroker::GetWEBManager().m_addonControl.UnregisterControl(m_id);
}

bool CWebAddonControl::RegisterGUIControl(CGUIWebAddonControl* control)
{
  const auto& entry = m_guiControls.find(control);
  if (entry != m_guiControls.end())
  {
    CLog::Log(LOGERROR, "CWebAddonControl::%s: Requested web control to register already present, for '%s'", __FUNCTION__, m_id.c_str());
    return false;
  }

  m_guiControls.insert(control);
  return true;
}

void CWebAddonControl::UnregisterGUIControl(CGUIWebAddonControl* control)
{
  const auto& entry = m_guiControls.find(control);
  if (entry != m_guiControls.end())
    m_guiControls.erase(entry);
}

bool CWebAddonControl::CreateWebControl(CGUIWebAddonControl* control, const std::string& startURL)
{
  const auto& entry = m_guiControls.find(control);
  if (entry == m_guiControls.end())
  {
    CLog::Log(LOGERROR, "CWebAddonControl::%s: Web control not present, for '%s'", __FUNCTION__, m_id.c_str());
    return false;
  }

  if (m_activeGUIControl != nullptr)
  {
    m_inactiveGUIControls.push_front(m_activeGUIControl);
    DestroyWebControl(m_activeGUIControl);
  }

  m_controlAddress = startURL;
  WEB_ADDON_ERROR err = m_addon->ControlCreate(control->GetProps(), startURL, &m_handle);
  if (err != WEB_ADDON_ERROR_NO_ERROR && err != WEB_ADDON_ERROR_NO_ERROR_REOPEN)
  {
    CLog::Log(LOGERROR, "CWebAddonControl::%s - Web addon control creation for '%s' failed", __FUNCTION__, m_id.c_str());
    return false;
  }

  m_activeGUIControl = control;
  m_activeGUIControl->UpdateOpenedAddress(m_controlAddress);
  if (m_firstGUIControl == nullptr)
    m_firstGUIControl = control;

  CLog::Log(LOGDEBUG, "CWebAddonControl::%s - Web addon control successfully created, for '%s'", __FUNCTION__, m_id.c_str());
  return true;
}

void CWebAddonControl::DestroyWebControl(CGUIWebAddonControl* control)
{
  if (m_activeGUIControl == nullptr)
    return;

  const auto& entry = m_guiControls.find(control);
  if (entry == m_guiControls.end())
    return;

  if (m_activeGUIControl != control)
  {
    if (m_inactiveGUIControls.empty())
    {
      CLog::Log(LOGERROR, "CWebAddonControl::%s - Destroy called from for '%s' from another control", __FUNCTION__, m_id.c_str());
      return;
    }
    else
    {
      for (unsigned int i = 0; i < m_inactiveGUIControls.size(); ++i)
      {
        if (m_inactiveGUIControls[i] == control)
        {
          m_inactiveGUIControls.erase(m_inactiveGUIControls.begin()+i);
          break;
        }
      }
      return;
    }
  }

  if (m_firstGUIControl == m_activeGUIControl)
  {
    if (!m_inactiveGUIControls.empty())
      m_firstGUIControl = m_inactiveGUIControls.front();
  }

  m_addon->DestroyControl(&m_handle, false);

  m_activeGUIControl = nullptr;
  if (!m_inactiveGUIControls.empty())
  {
    CGUIWebAddonControl* activeGUIControl = m_inactiveGUIControls.front();
    m_inactiveGUIControls.pop_front();
    CreateWebControl(activeGUIControl, "");
  }
}

// -----------------------------------------------------------------------------

void CWebAddonControl::Render()
{
  m_addon->Render(&m_handle);
}

bool CWebAddonControl::Dirty()
{
  return m_addon->Dirty(&m_handle);
}

bool CWebAddonControl::OnInit()
{
  return m_addon->OnInit(&m_handle);
}

bool CWebAddonControl::OnAction(int actionId, uint32_t buttoncode, wchar_t unicode, int& nextItem)
{
  return m_addon->OnAction(&m_handle, actionId, buttoncode, unicode, nextItem);
}

bool CWebAddonControl::OnMouseEvent(int id, double x, double y, double offsetX, double offsetY, int state)
{
  return m_addon->OnMouseEvent(&m_handle, id, x, y, offsetX, offsetY, state);
}

bool CWebAddonControl::GetHistory(std::vector<std::string>& historyWebsiteNames, bool behindCurrent)
{
  return m_addon->ControlGetHistory(&m_handle, historyWebsiteNames, behindCurrent);
}

void CWebAddonControl::SearchText(const std::string& text, bool forward, bool matchCase, bool findNext)
{
  m_addon->ControlSearchText(&m_handle, text, forward, matchCase, findNext);
}

void CWebAddonControl::StopSearch(bool clearSelection)
{
  m_addon->ControlStopSearch(&m_handle, clearSelection);
}

void CWebAddonControl::ScreenSizeChange(float x, float y, float width, float height, bool fullscreen)
{
  m_addon->ControlScreenSizeChange(&m_handle, x, y, width, height, fullscreen);
}

bool CWebAddonControl::OpenWebsite(const std::string& url)
{
  m_controlAddress = url;
  return m_addon->OpenWebsite(&m_handle, url);
}

void CWebAddonControl::Reload()
{
  m_addon->Reload(&m_handle);
}

void CWebAddonControl::StopLoad()
{
  m_addon->StopLoad(&m_handle);
}

void CWebAddonControl::GoBack()
{
  m_addon->GoBack(&m_handle);
}

void CWebAddonControl::GoForward()
{
  m_addon->GoForward(&m_handle);
}

void CWebAddonControl::OpenOwnContextMenu()
{
  m_addon->OpenOwnContextMenu(&m_handle);
}

// -----------------------------------------------------------------------------

void CWebAddonControl::SetControlReady(bool ready)
{
  OpenWebsite(m_controlAddress); // if empty use addon his default home url

  if (m_activeGUIControl)
    m_activeGUIControl->UpdateOpenedAddress(m_controlAddress);
}

void CWebAddonControl::SetOpenedAddress(const std::string &address)
{
  m_controlAddress = address;

  if (m_activeGUIControl)
    m_activeGUIControl->UpdateOpenedAddress(address);
}

void CWebAddonControl::SetOpenedTitle(const std::string &title)
{
  m_controlTitle = title;

  if (m_activeGUIControl)
    m_activeGUIControl->UpdateOpenedTitle(title);
}

void CWebAddonControl::SetIconURL(const std::string &icon)
{
  m_controlIcon = icon;

  if (m_activeGUIControl)
    m_activeGUIControl->UpdateIconURL(icon);
}

void CWebAddonControl::SetLoadingState(bool isLoading, bool canGoBack, bool canGoForward)
{
  m_isLoading = isLoading;
  m_canGoBack = canGoBack;
  m_canGoForward = canGoForward;
}

void CWebAddonControl::SetTooltip(const std::string &tooltip)
{
  m_tooltyp = tooltip;
}

void CWebAddonControl::SetStatusMessage(const std::string &statusMessage)
{
  m_statusMessage = statusMessage;
}

void CWebAddonControl::RequestOpenSiteInNewTab(const std::string &url)
{
  if (m_activeGUIControl)
    m_activeGUIControl->RequestOpenSiteInNewTab(url);
}
