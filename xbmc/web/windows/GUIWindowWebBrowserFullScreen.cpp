/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowWebBrowserFullScreen.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWebAddonControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/Key.h"
#include "web/dialogs/GUIDialogFavourites.h"
#include "web/windows/GUIWindowWebBrowser.h"

#define CONTROL_WEBADDON 2

using namespace WEB;

CGUIWindowWebBrowserFullScreen::CGUIWindowWebBrowserFullScreen(void) :
  CGUIWindow(WINDOW_WEB_BROWSER_FULLSCREEN, "VideoFullScreen.xml"),
  m_useRunningControl(false),
  m_webControl(nullptr)
{
}

void CGUIWindowWebBrowserFullScreen::OnInitWindow(void)
{
  CGUIWindow::OnInitWindow();

  CGUIWindowWebBrowser *runningWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowWebBrowser>(WINDOW_WEB_BROWSER);
  if (runningWindow)
  {
    m_webControl = runningWindow->GetRunningControl();
    if (m_webControl)
    {
      m_useRunningControl = true;
      m_webControl->SetFullscreen(true);
    }
    else
      m_useRunningControl = false;
  }

  if (!m_useRunningControl)
    m_webControl = new CGUIWebAddonControl(GetID(),
                                           CONTROL_WEBADDON,
                                           0, 0,
                                           GetWidth(),
                                           GetHeight());
  AddControl(m_webControl);
  SetDefaultControl(CONTROL_WEBADDON, true);
}

void CGUIWindowWebBrowserFullScreen::OnDeinitWindow(int nextWindowID)
{
  if (m_webControl)
  {
    RemoveControl(m_webControl);
    if (m_useRunningControl)
    {
      m_webControl->SetFullscreen(false);
      m_useRunningControl = false;
    }
    else
    {
      delete m_webControl;
      m_webControl = nullptr;
    }
  }
  CGUIWindow::OnDeinitWindow(nextWindowID);
}

#define CONTEXT_MENU_GO_BACK          0
#define CONTEXT_MENU_GO_FWD           1
#define CONTEXT_MENU_OPEN_URL         2
#define CONTEXT_MENU_RELOAD           3
#define CONTEXT_MENU_OPEN_FAVOURITES  4
#define CONTEXT_MENU_HOME             5
#define CONTEXT_MENU_CLOSE_FULLSCREEN 6
bool CGUIWindowWebBrowserFullScreen::OnAction(const CAction &action)
{
  if (m_webControl)
  {
    int id = action.GetID();
    switch (id)
    {
      case ACTION_CONTEXT_MENU:
      case ACTION_MOUSE_RIGHT_CLICK:
      {
        CContextButtons choices;
        if (m_webControl->CanGoBack())
          choices.Add(CONTEXT_MENU_GO_BACK, g_localizeStrings.Get(915));
        if (m_webControl->CanGoForward())
          choices.Add(CONTEXT_MENU_GO_FWD, g_localizeStrings.Get(916));
        choices.Add(CONTEXT_MENU_OPEN_URL, g_localizeStrings.Get(1051));
        choices.Add(CONTEXT_MENU_RELOAD, g_localizeStrings.Get(914));
        choices.Add(CONTEXT_MENU_OPEN_FAVOURITES, g_localizeStrings.Get(1036));
        choices.Add(CONTEXT_MENU_HOME, g_localizeStrings.Get(913));
        choices.Add(CONTEXT_MENU_CLOSE_FULLSCREEN, g_localizeStrings.Get(917));
        int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
        if (choice < 0)
          return false;

        switch (choice)
        {
          case CONTEXT_MENU_GO_BACK:
            CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_WEB_CONTROL_GO_BACK, GetID(), CONTROL_WEBADDON);
            break;
          case CONTEXT_MENU_GO_FWD:
            CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_WEB_CONTROL_GO_FWD, GetID(), CONTROL_WEBADDON);
            break;
          case CONTEXT_MENU_OPEN_URL:
          {
            std::string url = m_webControl->OpenedAddress();
            if (CGUIKeyboardFactory::ShowAndGetInput(url, CVariant{g_localizeStrings.Get(912)}, true) && !url.empty())
            {
              CGUIMessage msg(GUI_MSG_WEB_CONTROL_LOAD, GetID(), CONTROL_WEBADDON);
              msg.SetLabel(url);
              CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
            }
            break;
          }
          case CONTEXT_MENU_RELOAD:
            CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_WEB_CONTROL_RELOAD, GetID(), CONTROL_WEBADDON);
            break;
          case CONTEXT_MENU_OPEN_FAVOURITES:
          {
            std::string url = WEB::CGUIDialogWebFavourites::ShowAndGetURL();
            if (!url.empty())
            {
              CGUIMessage msg(GUI_MSG_WEB_CONTROL_LOAD, GetID(), CONTROL_WEBADDON);
              msg.SetLabel(url);
              CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
            }
            break;
          }
          case CONTEXT_MENU_HOME:
            CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_WEB_CONTROL_HOME, GetID(), CONTROL_WEBADDON);
            break;
          case CONTEXT_MENU_CLOSE_FULLSCREEN:
            CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
            break;
          default:
            return false;
        }
        return true;
      }
      default:
        break;
    }
  }

  return CGUIWindow::OnAction(action);
}
