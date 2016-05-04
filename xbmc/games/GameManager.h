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

    /*!
     * \brief Initialize the manager
    */
    void Start();

    /*!
    * \brief Deinitialize the manager
    */
    void Stop();

    /*!
    * \brief Get the instance of the specified add-on
    */
    bool GetAddonInstance(const std::string& strClientId, GameClientPtr& addon) const;

    /*!
     * \brief Select a game client, possibly via prompt, for the given game
     *
     * \param file The game being played
     *
     * \return A game client ready to be initialized for playback
     */
    GameClientPtr OpenGameClient(const CFileItem& file);

    /**
     * Get a list of valid game client extensions (as determined by the tag in
     * addon.xml). Includes game clients in remote repositories.
     */
    void GetExtensions(std::vector<std::string>& exts) const;

    /*!
     * \brief Check if the file extension is supported by an add-on in
     *        a local or remote repository
     *
     * \param path The path of the game file
     *
     * \return true if the path's extension is supported by a known game client
     */
    bool HasGameExtension(const std::string& path) const;

  private:
    void UpdateAddons();

    /*!
     * \brief Resolve a file item to a list of game clients
     */
    void GetGameClients(const CFileItem& file, GameClientVector& candidates, GameClientVector& installable) const;

    typedef std::string                           GameClientID;
    typedef std::map<GameClientID, GameClientPtr> GameClientMap;

    GameClientMap         m_gameClients;
    std::set<std::string> m_gameExtensions;
    CCriticalSection      m_critSection;
  };
} // namespace GAME
