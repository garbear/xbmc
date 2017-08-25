/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameSaves.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
//#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

CDialogGameSaves::CDialogGameSaves()
    : CDialogGameVideoSelect(WINDOW_DIALOG_GAME_SAVES)
{
}

std::string CDialogGameSaves::GetHeading()
{
  return g_localizeStrings.Get(35249); // "Saved games"
}

void CDialogGameSaves::PreInit()
{
  m_items.Clear();

  InitSavedGames();

  if (m_items.Size() == 0)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(231)); // "None"
    m_items.Add(std::move(item));
  }
}

void CDialogGameSaves::InitSavedGames()
{
  //! @todo
}

void CDialogGameSaves::GetItems(CFileItemList& items)
{
  for (const auto& item : m_items)
    items.Add(item);
}

void CDialogGameSaves::OnItemFocus(unsigned int index)
{
  if (static_cast<int>(index) < m_items.Size())
  {
    CFileItemPtr item = m_items[index];

    //! @todo
  }
}

unsigned int CDialogGameSaves::GetFocusedItem() const
{
  //! @todo

  return 0;
}

void CDialogGameSaves::PostExit()
{
  m_items.Clear();
}
