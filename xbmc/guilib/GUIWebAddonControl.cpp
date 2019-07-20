/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWebAddonControl.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "GUIComponent.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "filesystem/SpecialProtocol.h"
#include "input/Key.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "web/Favourites.h"
#include "web/WebManager.h"
#include "web/windows/GUIWindowWebBrowser.h"

using namespace WEB;

CGUIWebAddonControl::CGUIWebAddonControl(int parentID, int controlID, float posX, float posY, float width, float height,
                                         const std::string& startAddress)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_attemptedLoad(false),
    m_startAddress(startAddress),
    m_props{0},
    m_transparentBackground(false),
    m_singleReload(true),
    m_isFullScreen(false)
{
  ControlType = GUICONTROL_WEB_ADDON;
}

CGUIWebAddonControl::CGUIWebAddonControl(const CGUIWebAddonControl& from)
  : CGUIControl(from),
    m_attemptedLoad(false),
    m_startAddress(from.m_startAddress),
    m_props{0},
    m_transparentBackground(from.m_transparentBackground),
    m_singleReload(from.m_singleReload),
    m_isFullScreen(from.m_isFullScreen)
{
  ControlType = GUICONTROL_WEB_ADDON;
}

CGUIWebAddonControl::~CGUIWebAddonControl()
{
  if (m_addonControl != nullptr)
    m_addonControl->UnregisterGUIControl(this);
}

void CGUIWebAddonControl::UpdateInfo(const CGUIListItem* item)
{
  if (item && m_bInvalidated)
  {
    if (m_addonControl == nullptr)
    {
      m_idString = item->GetProperty("browseridstring").asString();
      if (m_idString.empty())
      {
        CLog::Log(LOGERROR, "CGUIWebAddonControl::{}: Called without defined 'browseridstring' for '{}'", __FUNCTION__, item->GetLabel2());
        return;
      }

      m_addonControl = CServiceBroker::GetWEBManager().m_addonControl.GetControl(m_idString);
      if (m_addonControl == nullptr)
      {
        CLog::Log(LOGERROR, "CGUIWebAddonControl::{}: Failed to get web addon control!", __FUNCTION__);
        m_attemptedLoad = true;
        return;
      }
      m_addonControl->RegisterGUIControl(this);
    }

    CGUIWebAddonControl* control = m_addonControl->GetFirstControl();
    if (control == nullptr)
      return;

    m_idString = control->m_idString;
    m_props = control->m_props;
    m_attemptedLoad = false;
  }
}

bool CGUIWebAddonControl::OnMessage(CGUIMessage &message)
{
  if (m_addonControl && message.GetControlId() == GetID())
  {
    switch (message.GetMessage())
    {
    case GUI_MSG_WEB_CONTROL_GO_BACK:
    {
      m_addonControl->GoBack();
      return true;
    }
    case GUI_MSG_WEB_CONTROL_GO_FWD:
    {
      m_addonControl->GoForward();
      return true;
    }
    case GUI_MSG_WEB_CONTROL_LOAD:
    {
      std::string url = message.GetLabel();
      m_addonControl->OpenWebsite(url);
      m_loadAddress = url;
      return true;
    }
    case GUI_MSG_WEB_CONTROL_RELOAD:
    {
      std::string url = message.GetLabel();
      if (m_singleReload && url != m_addonControl->OpenedAddress())
        m_addonControl->OpenWebsite(url);
      else
        m_addonControl->Reload();
      return true;
    }
    case GUI_MSG_WEB_CONTROL_HOME:
    {
      std::string startAddress = GenerateAddress(m_startAddress);
      m_addonControl->OpenWebsite(startAddress);
      return true;
    }
    case GUI_MSG_WEB_CONTROL_SETTINGS:
    {
      m_addonControl->OpenOwnContextMenu();
      return true;
    }
    case GUI_MSG_WEB_CONTROL_ADD_TO_FAVOURITES:
    {
      CWebFavourites::AddToFavourites(m_addonControl->OpenedTitle(), m_addonControl->OpenedAddress(), m_addonControl->IconURL());
      return true;
    }
    default:
      break;
    }
  }

  return CGUIControl::OnMessage(message);
}

EVENT_RESULT CGUIWebAddonControl::OnMouseEvent(const CPoint& point, const CMouseEvent& event)
{
  if (m_addonControl && m_addonControl->OnMouseEvent(event.m_id, point.x, point.y, event.m_offsetX, event.m_offsetY, event.m_state))
    return EVENT_RESULT_HANDLED;

  return EVENT_RESULT_UNHANDLED;
}

bool CGUIWebAddonControl::OnAction(const CAction &action)
{
  int nextItem = -1;
  if (!m_addonControl || !m_addonControl->OnAction(action.GetID(), action.GetButtonCode(), action.GetUnicode(), nextItem))
    return CGUIControl::OnAction(action);

  if (nextItem > 0)
  {
    // Advance the selected item one notch
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), nextItem);
    OnMessage(msg);
  }

  return true;
}

void CGUIWebAddonControl::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  /* Load the addon on first call */
  if (m_addonControl == nullptr && !m_attemptedLoad)
  {
    if (m_idString.empty())
      m_idString = StringUtils::CreateUUID();

    m_addonControl = CServiceBroker::GetWEBManager().m_addonControl.GetControl(m_idString);
    if (m_addonControl == nullptr)
    {
      CLog::Log(LOGERROR, "CGUIWebAddonControl::{}: Failed to get web addon control!", __FUNCTION__);
      m_attemptedLoad = true;
      return;
    }
    m_addonControl->RegisterGUIControl(this);
  }

  if (m_addonControl && !m_attemptedLoad)
  {
    m_props.fSkinXPos = GetXPosition();
    m_props.fSkinYPos = GetYPosition();
    if (m_props.fSkinWidth == 0.0f)
      m_props.fSkinWidth = GetWidth();
    if (m_props.fSkinHeight == 0.0f)
      m_props.fSkinHeight = GetHeight();

    CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock();
    float x = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(GetXPosition(), GetYPosition());
    float y = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(GetXPosition(), GetYPosition());
    float w = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(GetXPosition() + m_props.fSkinWidth, GetYPosition() + m_props.fSkinHeight) - x;
    float h = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(GetXPosition() + m_props.fSkinWidth, GetYPosition() + m_props.fSkinHeight) - y;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth()) w = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth() - x;
    if (y + h > CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight()) h = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight() - y;

    m_props.strName = m_idString.c_str();
    m_props.pDevice = CServiceBroker::GetWinSystem()->GetHWContext();
    m_props.fXPos = x + 0.5f;
    m_props.fYPos = y + 0.5f;
    m_props.fWidth = w + 0.5f;
    m_props.fHeight = h + 0.5f;

    m_props.fPixelRatio = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().fPixelRatio;
    m_props.fFPS  = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
    m_props.bUseTransparentBackground = m_transparentBackground;
    m_props.iBackgroundColorARGB = static_cast<uint32_t>(m_diffuseColor);
    m_props.iGUIItemLeft = GetAction(ACTION_MOVE_LEFT).GetNavigation();
    m_props.iGUIItemRight = GetAction(ACTION_MOVE_RIGHT).GetNavigation();
    m_props.iGUIItemTop = GetAction(ACTION_MOVE_UP).GetNavigation();
    m_props.iGUIItemBottom = GetAction(ACTION_MOVE_DOWN).GetNavigation();
    m_props.iGUIItemBack = GetAction(ACTION_NAV_BACK).GetNavigation();
    m_props.pControlIdent = m_addonControl.get();

    std::string startAddress = GenerateAddress(m_loadAddress.empty() ? m_startAddress : m_loadAddress);
    if (m_addonControl->CreateWebControl(this, startAddress))
    {
      CLog::Log(LOGDEBUG, "{} - Web addon control successfully created, for '{}'", __FUNCTION__, m_idString);
      CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
    }
    else
    {
      CLog::Log(LOGERROR, "{} - Web addon control creation for '{}' failed", __FUNCTION__, m_idString);
      m_addonControl.reset();
    }

    m_attemptedLoad = true;
  }

  // TODO Add processing to the addon so it could mark when actually changing
  if (m_addonControl && m_addonControl->Dirty())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIWebAddonControl::Render()
{
  if (m_addonControl &&
      static_cast<int>(m_renderRegion.x1) < CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth() &&
      static_cast<int>(m_renderRegion.y1) < CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight() &&
      m_renderRegion.x2 >= 0.0f &&
      m_renderRegion.y2 >= 0.0f) // Do not render if control is outside of visible view (brings a badly fullscreen of a small control)
  {
    // set the viewport - note: We currently don't have any control over how
    // the addon renders, so the best we can do is attempt to define
    // a viewport??
    CServiceBroker::GetWinSystem()->GetGfxContext().SetViewPort(m_posX, m_posY, m_width, m_height);
    CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock();
    m_addonControl->Render();
    CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreViewPort();
  }

  CGUIControl::Render();
}

void CGUIWebAddonControl::FreeResources(bool immediately)
{
  // tell our app that we're going
  if (!m_addonControl)
    return;

  // Destroy the addon control side only partly with the "false" to allow
  // background work.
  CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock(); //TODO locking
  m_addonControl->DestroyWebControl(this);
  CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();

  CGUIControl::FreeResources(immediately);

  m_attemptedLoad = false;
}

void CGUIWebAddonControl::UpdateVisibility(const CGUIListItem *item)
{
  // if made invisible, start timer, only free addonptr after
  // some period, configurable by window class
  CGUIControl::UpdateVisibility(item);
  if (!IsVisible())
    FreeResources();
}

bool CGUIWebAddonControl::CanFocusFromPoint(const CPoint &point) const
{
  // mouse is allowed to focus this control, but it doesn't actually receive focus
  return IsVisible() && HitTest(point);
}

/// Set functions for CGUIControlFactory
//@{
void CGUIWebAddonControl::SetWebControlIdString(const std::string& id)
{
  m_idString = id;
}

void CGUIWebAddonControl::SetTransparent(bool transparent)
{
  m_transparentBackground = transparent;
}

void CGUIWebAddonControl::SetReloadButton(bool singleReload)
{
  m_singleReload = singleReload;
}

void CGUIWebAddonControl::SetFullscreen(bool enable)
{
  if (!m_addonControl)
    return;

  if (enable && !m_isFullScreen)
  {
    m_windowedXPos = GetXPosition();
    m_windowedYPos = GetYPosition();
    m_windowedWidth = GetWidth();
    m_windowedHeight = GetHeight();
    m_windowedAddonXPos = m_props.fXPos;
    m_windowedAddonYPos = m_props.fYPos;
    m_windowedAddonWidth = m_props.fWidth;
    m_windowedAddonHeight = m_props.fHeight;
    m_windowedSkinXPos = m_props.fSkinXPos;
    m_windowedSkinYPos = m_props.fSkinYPos;
    m_windowedSkinWidth = m_props.fSkinWidth;
    m_windowedSkinHeight = m_props.fSkinHeight;

    m_props.fXPos = 0;
    m_props.fYPos = 0;
    m_props.fWidth = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
    m_props.fHeight = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
    m_props.fSkinXPos = 0;
    m_props.fSkinYPos = 0;
    m_props.fSkinWidth = m_props.fWidth;
    m_props.fSkinHeight = m_props.fHeight;
    m_isFullScreen = true;

    SetPosition(m_props.fXPos, m_props.fYPos);
    SetWidth(m_props.fWidth);
    SetHeight(m_props.fHeight);
  }
  else if (!enable && m_isFullScreen)
  {
    m_props.fXPos = m_windowedAddonXPos;
    m_props.fYPos = m_windowedAddonYPos;
    m_props.fWidth = m_windowedAddonWidth;
    m_props.fHeight = m_windowedAddonHeight;
    m_props.fSkinXPos = m_windowedSkinXPos;
    m_props.fSkinYPos = m_windowedSkinYPos;
    m_props.fSkinWidth = m_windowedSkinWidth;
    m_props.fSkinHeight = m_windowedSkinHeight;
    m_isFullScreen = false;

    SetPosition(m_windowedXPos, m_windowedYPos);
    SetWidth(m_windowedWidth);
    SetHeight(m_windowedHeight);
  }

  m_addonControl->ScreenSizeChange(m_props.fXPos, m_props.fYPos, m_props.fWidth, m_props.fHeight, m_isFullScreen);
}
//@}

/// Interact functions between CGUIWebAddonControl and CWebAddonControl
//@{
void CGUIWebAddonControl::UpdateOpenedAddress(const std::string& address)
{
  CGUIMessage msg(GUI_MSG_WEB_UPDATE_ADDRESS, GetID(), GetParentID());
  msg.SetLabel(address);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CGUIWebAddonControl::UpdateOpenedTitle(const std::string& title)
{
  CGUIMessage msg(GUI_MSG_WEB_UPDATE_NAME, GetID(), GetParentID());
  msg.SetLabel(title);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CGUIWebAddonControl::UpdateIconURL(const std::string& icon)
{
  CGUIMessage msg(GUI_MSG_WEB_UPDATE_ICON, GetID(), GetParentID());
  msg.SetLabel(icon);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CGUIWebAddonControl::RequestOpenSiteInNewTab(const std::string& url)
{
  CGUIMessage msg(GUI_MSG_WEB_NEW_TAB, GetID(), GetParentID());
  msg.SetLabel(url);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}
//@}

/// GUI interface info functions (used from places where control is on window)
//@{
bool CGUIWebAddonControl::IsLoading() const
{
  if (!m_addonControl)
    return false;

  return m_addonControl->IsLoading();
}

bool CGUIWebAddonControl::CanGoBack() const
{
  if (!m_addonControl)
    return false;

  return m_addonControl->CanGoBack();
}

bool CGUIWebAddonControl::CanGoForward() const
{
  if (!m_addonControl)
    return false;

  return m_addonControl->CanGoForward();
}

const std::string &CGUIWebAddonControl::OpenedAddress() const
{
  if (!m_addonControl)
    return StringUtils::Empty;

  std::string url = m_addonControl->OpenedAddress();
  if (url.empty())
    return m_loadAddress.empty() ? m_startAddress : m_loadAddress;

  return m_addonControl->OpenedAddress();
}

const std::string &CGUIWebAddonControl::OpenedTitle() const
{
  if (!m_addonControl)
    return StringUtils::Empty;

  return m_addonControl->OpenedTitle();
}

const std::string &CGUIWebAddonControl::IconURL() const
{
  if (!m_addonControl)
    return StringUtils::Empty;

  return m_addonControl->IconURL();
}

const std::string &CGUIWebAddonControl::StatusMessage() const
{
  if (!m_addonControl)
    return StringUtils::Empty;

  return m_addonControl->StatusMessage();
}

const std::string &CGUIWebAddonControl::Tooltip() const
{
  if (!m_addonControl)
    return StringUtils::Empty;

  return m_addonControl->Tooltip();
}

void CGUIWebAddonControl::SetLoadAddress(const std::string& loadAddress)
{
  m_loadAddress = loadAddress;
}

bool CGUIWebAddonControl::GetHistory(std::vector<std::string>& historyWebsiteNames, bool behindCurrent)
{
  if (!m_addonControl)
    return false;

  return m_addonControl->GetHistory(historyWebsiteNames, behindCurrent);
}

void CGUIWebAddonControl::SearchText(const std::string& text, bool forward, bool matchCase, bool findNext)
{
  if (!m_addonControl)
    return;

  m_addonControl->SearchText(text, forward, matchCase, findNext);
}

void CGUIWebAddonControl::StopSearch(bool clearSelection)
{
  if (!m_addonControl)
    return;

  m_addonControl->StopSearch(clearSelection);
}
//@}

void CGUIWebAddonControl::SetWebAddonInvalid()
{
  if (m_addonControl != nullptr)
    m_addonControl->UnregisterGUIControl(this);
  m_addonControl = nullptr;
}

std::string CGUIWebAddonControl::GenerateAddress(const std::string& startAddress)
{
  // if empty do not check, empty means start URL
  if (startAddress.empty())
    return startAddress;

  std::string address = CSpecialProtocol::TranslatePath(startAddress);
  if (!URIUtils::IsURL(address))
  {
    address = "file://" + address;
  }

  return address;
}
