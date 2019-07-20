/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogFavourites.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogYesNo.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "web/WebDatabase.h"

#define CONTROL_NUMBER_OF_ITEMS 2
#define CONTROL_SIMPLE_LIST     3
#define CONTROL_DETAILED_LIST   6
#define CONTROL_EXTRA_BUTTON    5
#define CONTROL_CANCEL_BUTTON   7

namespace WEB
{

CGUIDialogWebFavourites::CGUIDialogWebFavourites()
    : CGUIDialogBoxBase(WINDOW_DIALOG_WEB_FAVOURITES, "DialogSelect.xml"),
    m_selectedItem(-1),
    m_vecList(new CFileItemList())
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogWebFavourites::~CGUIDialogWebFavourites(void) = default;

std::string CGUIDialogWebFavourites::ShowAndGetFavourite(CWebDatabase* database)
{
  std::vector<FavouriteEntryPtr> entries;
  m_database = database;
  if (m_database->Open() &&
      m_database->GetFavourites(entries))
  {
    for (const auto& entry : entries)
    {
      CFileItemPtr item(std::make_shared<CFileItem>(entry->GetName()));
      item->SetLabel2(entry->GetURL());
      item->SetIconImage(entry->GetIcon());
      m_vecList->Add(item);
    }
    m_vecList->Sort(SortByLabel, SortOrderAscending);

    Open();

    std::string url;
    if (m_selectedItem >= 0)
      url = m_vecList->Get(m_selectedItem)->GetLabel2();

    m_vecList->Clear();
    m_database = nullptr;
    return url;
  }

  return StringUtils::Empty;
}

std::string CGUIDialogWebFavourites::ShowAndGetURL()
{
  CGUIDialogWebFavourites *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogWebFavourites>(WINDOW_DIALOG_WEB_FAVOURITES);
  if (dialog)
  {
    CWebDatabase database;
    return dialog->ShowAndGetFavourite(&database);
  }
  return StringUtils::Empty;
}

bool CGUIDialogWebFavourites::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialogBoxBase::OnMessage(message);
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_selectedItem = -1;
      SetHeading(CVariant{g_localizeStrings.Get(1036)});
      CGUIDialogBoxBase::OnMessage(message);
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_CANCEL_BUTTON)
      {
        m_selectedItem = -1;
        Close();
      }
      else
      {
        int action = message.GetParam1();
        switch (action)
        {
          case ACTION_SELECT_ITEM:
          case ACTION_MOUSE_LEFT_CLICK:
          {
            int iSelected = m_viewControl.GetSelectedItem();
            if (iSelected >= 0 && iSelected < m_vecList->Size())
            {
              CFileItemPtr item(m_vecList->Get(iSelected));
              for (int i = 0 ; i < m_vecList->Size() ; i++)
                m_vecList->Get(i)->Select(false);
              item->Select(true);
              m_selectedItem = iSelected;
              Close();
            }
            break;
          }
          case ACTION_CONTEXT_MENU:
          case ACTION_MOUSE_RIGHT_CLICK:
          {
            int iSelected = m_viewControl.GetSelectedItem();
            if (iSelected >= 0 && iSelected < m_vecList->Size())
            {
              OnPopupMenu(iSelected);
              return true;
            }
            break;
          }
          default:
            break;
        }
      }
    }
    break;
  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()))
      {
        if (m_vecList->IsEmpty())
        {
          SET_CONTROL_FOCUS(CONTROL_CANCEL_BUTTON, 0);
          return true;
        }
        if (m_viewControl.GetCurrentControl() != message.GetControlId())
        {
          m_viewControl.SetFocused();
          return true;
        }
      }
    }
    break;
  }

  return CGUIDialogBoxBase::OnMessage(message);
}

bool CGUIDialogWebFavourites::OnBack(int actionID)
{
  m_selectedItem = -1;
  return CGUIDialogBoxBase::OnBack(actionID);
}

CGUIControl *CGUIDialogWebFavourites::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIDialogBoxBase::GetFirstFocusableControl(id);
}

void CGUIDialogWebFavourites::OnWindowLoaded()
{
  CGUIDialogBoxBase::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_SIMPLE_LIST));
  m_viewControl.AddView(GetControl(CONTROL_DETAILED_LIST));
}

void CGUIDialogWebFavourites::OnInitWindow()
{
  m_viewControl.SetItems(*m_vecList);
  m_selectedItem = m_vecList->Size() >= 0 ? 0 : -1;
  m_viewControl.SetCurrentView(CONTROL_DETAILED_LIST);

  SET_CONTROL_LABEL(CONTROL_NUMBER_OF_ITEMS, StringUtils::Format("%i %s", m_vecList->Size(), g_localizeStrings.Get(127).c_str()));
  SET_CONTROL_HIDDEN(CONTROL_EXTRA_BUTTON);
  SET_CONTROL_LABEL(CONTROL_CANCEL_BUTTON, g_localizeStrings.Get(15067));

  CGUIDialogBoxBase::OnInitWindow();

  // if nothing is selected, select first item
  m_viewControl.SetSelectedItem(0);
}

void CGUIDialogWebFavourites::OnDeinitWindow(int nextWindowID)
{
  m_viewControl.Clear();
  CGUIDialogBoxBase::OnDeinitWindow(nextWindowID);
}

void CGUIDialogWebFavourites::OnWindowUnload()
{
  CGUIDialogBoxBase::OnWindowUnload();
  m_viewControl.Reset();
}

void CGUIDialogWebFavourites::OnPopupMenu(int item)
{
  if (item < 0 || item >= m_vecList->Size())
    return;

  CContextButtons choices;
  choices.Add(1, 14072);  // Open favourite
  choices.Add(2, 14077);  // Remove from favourites
  choices.Add(3, 14073);  // Remove all favourites
  int button = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  if (button == 1)
  {
    m_selectedItem = item;
    Close();
    return;
  }
  else if (button == 2)
  {
    // prompt user to be sure
    if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{14077}, CVariant{750}))
      return;

    if (m_database->RemoveFavourite(m_vecList->Get(item)->GetLabel(), m_vecList->Get(item)->GetLabel2()))
    {
      m_vecList->Remove(item);
      m_viewControl.SetItems(*m_vecList);
    }
  }
  else if (button == 3)
  {
    // prompt user to be sure
    if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{14073}, CVariant{750}))
      return;

    m_database->ClearFavourites();
    Close();
  }
}

} /* namespace WEB */
