/*
 *  Copyright (C) 2012-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSelectSavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "dialogs/GUIDialogSelect.h"
#include "FileItem.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

bool CGUIDialogSelectSavestate::ShowAndGetSavestate(const std::string& gamePath,
                                                    std::string& savestatePath)
{
  RETRO::CSavestateDatabase db;
  CFileItemList items;
  bool bSuccess = false;

  savestatePath = "";

  if (!db.GetSavestatesNav(items, gamePath))
    return bSuccess;

  LogSavestates(items);

  for (int i = 0; i < items.Size(); i++)
  {
    items[i]->SetLabel(items[i]->m_dateTime.GetAsLocalizedDateTime());
    items[i]->SetIconImage(db.MakeThumbnailPath(items[i]->GetPath()));
  }

  items.Sort(SortByDate, SortOrderDescending);

  // if there are no saves there is no need to prompt the user
  if (items.Size() == 0)
  {
    savestatePath = "";
    return true;
  }
  else
  {
    // "Select savestate"
    CGUIDialogSelect* dialog = GetDialog(g_localizeStrings.Get(35259));

    if (dialog != nullptr)
    {
      dialog->SetItems(items);
      dialog->Open();

      if (dialog->IsConfirmed())
      {
        int selectedIndex = dialog->GetSelectedItem();

        if (selectedIndex >= 0 && selectedIndex <= items.Size())
        {
          savestatePath = items[selectedIndex]->GetPath();
          bSuccess = true;
        }
        else
        {
          CLog::Log(LOGDEBUG, "Select savestate dialog: User selected invalid savestate %d",
                    selectedIndex);
        }
      }
      else if (dialog->IsButtonPressed())
      {
        CLog::Log(LOGDEBUG, "Select savestate dialog: New savestate selected");
        bSuccess = true;
      }
    }
  }

  return bSuccess;
}

CGUIDialogSelect* CGUIDialogSelectSavestate::GetDialog(const std::string& title)
{
  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);

  if (dialog != nullptr)
  {
    dialog->Reset();
    dialog->SetHeading(CVariant{title});
    dialog->SetUseDetails(true);
    dialog->EnableButton(true, 223); // "New"
  }

  return dialog;
}

void CGUIDialogSelectSavestate::LogSavestates(const CFileItemList& items)
{
  CLog::Log(LOGDEBUG, "Select savestate dialog: Found %d saves", items.Size());
  for (int i = 0; i < items.Size(); i++)
  {
    CLog::Log(LOGDEBUG, "%s", items[i]->GetPath());
  }
}