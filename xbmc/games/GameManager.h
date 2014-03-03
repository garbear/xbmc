/*
 *      Copyright (C) 2012-2016 Team Kodi
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
#pragma once

#include "GameFileAutoLauncher.h"
#include "GameTypes.h"
#include "addons/AddonManager.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <map>
#include <set>
#include <string>

class FileItem;

namespace GAME
{
  class CGameManager : public ADDON::IAddonMgrCallback,
                       public Observer
  {
  public:
    CGameManager() { }

    virtual ~CGameManager() { Stop(); }

    static CGameManager& GetInstance();
    
    // implementation of IAddonMgrCallback
    virtual bool RequestRestart(ADDON::AddonPtr addon, bool datachanged) { return true; }
    virtual bool RequestRemoval(ADDON::AddonPtr addon); // TODO: Never called!

    // implementation of Observer
    virtual void Notify(const Observable& obs, const ObservableMessage msg) override;

    void Start();
    void Stop();

    bool GetAddonInstance(const std::string& strClientId, GameClientPtr& addon) const;

    /**
     * Resolve a file item to a list of game clients
     */
    void GetGameClients(const CFileItem& file, GameClientVector& candidates) const;

    /**
     * Get a list of valid game client extensions (as determined by the tag in
     * addon.xml). Includes game clients in remote repositories.
     */
    void GetExtensions(std::vector<std::string>& exts) const;

    /**
     * Returns true if the file extension is supported by an add-on in an enabled
     * repository.
     */
    bool IsGame(const std::string& path) const;

    // Queue a file to be launched when the next game client is installed.
    void SetAutoLaunch(const CFileItem& file) { m_fileLauncher.SetAutoLaunch(file); }
    void ClearAutoLaunch()                    { m_fileLauncher.ClearAutoLaunch(); }

  private:
    void UpdateAddons();

    typedef std::string                           GameClientID;
    typedef std::map<GameClientID, GameClientPtr> GameClientMap;

    GameClientMap         m_gameClients;
    std::set<std::string> m_gameExtensions;
    CGameFileAutoLauncher m_fileLauncher;
    CCriticalSection      m_critSection;
  };
} // namespace GAME
