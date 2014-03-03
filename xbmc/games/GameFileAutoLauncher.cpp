/*
 *      Copyright (C) 2013-2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameFileAutoLauncher.h"
#include "Application.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/addons/GameClient.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <string>

#define UPDATE_INTERVAL_MS   5 * 1000 // 5s
#define TIMEOUT_MS           2 * 60 * 1000 // 2 mins

using namespace GAME;

CGameFileAutoLauncher::CGameFileAutoLauncher() : CThread("Game launcher")
{
}

CGameFileAutoLauncher::~CGameFileAutoLauncher()
{
  StopThread();
}

void CGameFileAutoLauncher::Process()
{
  m_timeout.Set(TIMEOUT_MS);

  // Loop continuously until StopThread() is called
  CEvent dummy;
  while (!(AbortableWait(dummy, UPDATE_INTERVAL_MS) == WAIT_INTERRUPTED || m_bStop))
  {
    // Otherwise, AbortableWait() returned WAIT_TIMEDOUT (the wait timed out)
    // Check state and wait again
    CSingleLock lock(m_critSection);
    if (!m_queuedFile || !IsStateValid())
    {
      CLog::Log(LOGDEBUG, "GameFileLauncher: aborted launch");
      m_queuedFile.reset();
      break;
    }
  }
  // StopThread() was called or state became invalid
}

void CGameFileAutoLauncher::SetAutoLaunch(const CFileItem& file)
{
  if (m_queuedFile)
    StopThread(true);

  {
    CSingleLock lock(m_critSection);
    m_queuedFile = CFileItemPtr(new CFileItem(file));
  }

  CLog::Log(LOGDEBUG, "GameFileLauncher: queued %s", file.GetPath().c_str());

  Create();
}

void CGameFileAutoLauncher::ClearAutoLaunch()
{
  {
    CSingleLock lock(m_critSection);
    m_queuedFile.reset();
  }

  StopThread(false);
}

bool CGameFileAutoLauncher::IsStateValid()
{
  // Check timeout
  if (m_timeout.IsTimePast())
    return false;

  // Check if the user is still in the add-on browser
  if (!g_windowManager.IsWindowActive(WINDOW_ADDON_BROWSER))
    return false;

  return true;
}

void CGameFileAutoLauncher::Launch(const GameClientPtr& gameClient)
{
  CFileItemPtr file;
  {
    CSingleLock lock(m_critSection);
    if (!m_queuedFile || !gameClient)
      return;
    file = m_queuedFile;
  }

  if (!gameClient->CanOpen(*file))
    return;

  CGUIDialogYesNo *pDialog = dynamic_cast<CGUIDialogYesNo*>(g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO));
  if (!pDialog)
    return;

  CLog::Log(LOGDEBUG, "GameFileAutoLauncher: prompting user for launch of %s", file->GetPath().c_str());

  std::string strTitle;
  if (file->HasGameInfoTag())
    strTitle = file->GetGameInfoTag()->GetTitle();
  if (strTitle.empty())
    strTitle = URIUtils::GetFileName(file->GetPath());

  /* TODO
  pDialog->SetHeading(27005); // Manage emulators...
  pDialog->SetLine(0, 27004); // A compatible emulator was installed for:
  pDialog->SetLine(1, strTitle);
  pDialog->SetLine(2, 20013); // Do you wish to launch the game?
  pDialog->DoModal();

  if (pDialog->IsConfirmed())
  {
    // Close the add-on info dialog, if open
    int iWindow = g_windowManager.GetTopMostModalDialogID(true);
    CGUIWindow* window = g_windowManager.GetWindow(iWindow);
    if (window)
      window->Close();

    // Force game client (so we aren't prompted for game client list by PlayMedia())
    file->SetProperty("Addon.ID", gameClient->ID());

    g_application.PlayMedia(*file);
  }
  */

  ClearAutoLaunch(); // Don't ask to launch the same file twice
}
