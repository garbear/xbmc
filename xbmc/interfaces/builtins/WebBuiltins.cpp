/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebBuiltins.h"

#include "GUIUserMessages.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/WindowTranslator.h"
#include "utils/StringUtils.h"
#include "web/dialogs/GUIDialogFavourites.h"
#include "web/windows/GUIWindowWebBrowser.h"

/*! \brief Send a message to a web control.
 *  \param params The parameters
 *  \details params[0] = ID of control.
 *           params[1] = Action name.
 *           params[2] = ID of window with control (optional).
 */
static int WebControlMessage(const std::vector<std::string>& params)
{
  std::string paramlow(params[1]);
  StringUtils::ToLower(paramlow);
  int controlID = atoi(params[0].c_str());
  int windowID = (params.size() == 3) ? CWindowTranslator::TranslateWindow(params[2]) : CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (paramlow == "back")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_WEB_CONTROL_GO_BACK, windowID, controlID);
  else if (paramlow == "fwd")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_WEB_CONTROL_GO_FWD, windowID, controlID);
  else if (paramlow == "load")
  {
    WEB::CGUIWindowWebBrowser *window = dynamic_cast<WEB::CGUIWindowWebBrowser*>(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(windowID));
    if (!window)
      return 0;
    CGUIMessage msg(GUI_MSG_WEB_CONTROL_LOAD, windowID, controlID);
    msg.SetLabel(window->SelectedAddress());
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }
  else if (paramlow == "reload")
  {
    CGUIMessage msg(GUI_MSG_WEB_CONTROL_LOAD, windowID, controlID);
    WEB::CGUIWindowWebBrowser *window = dynamic_cast<WEB::CGUIWindowWebBrowser*>(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(windowID));
    if (window)
      msg.SetLabel(window->SelectedAddress());
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }
  else if (paramlow == "favourites")
  {
    std::string url = WEB::CGUIDialogWebFavourites::ShowAndGetURL();
    if (!url.empty())
    {
      CGUIMessage msg(GUI_MSG_WEB_CONTROL_LOAD, windowID, controlID);
      msg.SetLabel(url);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }
  }
  else if (paramlow == "addtofavourites")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_WEB_CONTROL_ADD_TO_FAVOURITES, windowID, controlID);
  else if (paramlow == "home")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_WEB_CONTROL_HOME, windowID, controlID);
  else if (paramlow == "settings")
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_WEB_CONTROL_SETTINGS, windowID, controlID);

  return 0;
}

CBuiltins::CommandMap CWebBuiltins::GetOperations() const
{
  return {
           {"webcontrolcmd.message",       {"Send a given message to a control within a given web control", 1, WebControlMessage}}
         };
}
