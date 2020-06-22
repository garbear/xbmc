/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogInGameSaves.h"
#include "cores/RetroPlayer/playback/IPlayback.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "ServiceBroker.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"
//#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;
using namespace RETRO;

CDialogInGameSaves::CDialogInGameSaves()
    : CDialogGameVideoSelect(WINDOW_DIALOG_IN_GAME_SAVES)
{
}

std::string CDialogInGameSaves::GetHeading()
{
  return g_localizeStrings.Get(35249); // "Saved games"
}

void CDialogInGameSaves::PreInit()
{
  m_items.Clear();

  InitSavedGames();

  CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(223)); // "New"
  item->SetIconImage("DefaultAddSource.png");
  item->SetPath("");

  m_items.AddFront(std::move(item), 0);
}

void CDialogInGameSaves::InitSavedGames()
{
  auto gameSettings = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();

  // save current game
  IPlayback* playback = gameSettings->GetPlayback();
  playback->CreateSavestate();

  CSavestateDatabase db;
  db.GetSavestatesNav(m_items, gameSettings->GetPlayingGame(), gameSettings->GameClientID());

  for (int i = 0; i < m_items.Size(); i++)
  {
    std::unique_ptr<ISavestate> savestate = db.CreateSavestate();
    db.GetSavestate(m_items[i]->GetPath(), *savestate);

    const std::string label = savestate->Label();
    if (label.empty())
      m_items[i]->SetLabel(m_items[i]->m_dateTime.GetAsLocalizedDateTime());
    else
    {
      m_items[i]->SetLabel(label);
      m_items[i]->SetLabel2(m_items[i]->m_dateTime.GetAsLocalizedDateTime());
    }

    m_items[i]->SetIconImage(db.MakeThumbnailPath(m_items[i]->GetPath()));
  }

  m_items.Sort(SortByDate, SortOrderDescending);
}

void CDialogInGameSaves::GetItems(CFileItemList& items)
{
  for (const auto& item : m_items)
    items.Add(item);
}

void CDialogInGameSaves::OnItemFocus(unsigned int index)
{
  if (static_cast<int>(index) < m_items.Size())
    m_focusedItemIndex = index;
}

unsigned int CDialogInGameSaves::GetFocusedItem() const
{
  return m_focusedControl;
}

void CDialogInGameSaves::PostExit()
{
  m_items.Clear();
}

void CDialogInGameSaves::OnClickAction()
{
  if (static_cast<int>(m_focusedItemIndex) < m_items.Size())
  {
    auto gameSettings = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
    IPlayback* playback = gameSettings->GetPlayback();
    std::string savePath = m_items[m_focusedItemIndex]->GetPath(); 

    if (savePath.empty())
    {
      playback->ResetLoadedSave();
      gameSettings->ResetGame();
    }
    else
      playback->LoadSavestate(savePath);
  }
}
