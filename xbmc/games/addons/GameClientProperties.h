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

#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"

#include <string>
#include <vector>

struct game_client_properties;

namespace GAME
{

class CGameClient;

/**
 * \brief Wrapper for game client properties declared in kodi_game_types.h
 */
class CGameClientProperties
{
public:
  CGameClientProperties(const CGameClient* parent, game_client_properties*& props);
  ~CGameClientProperties(void) { ReleaseResources(); }

  void InitializeProperties(void);

private:
  // Release mutable resources
  void ReleaseResources(void);

  // Equal to parent's real library path
  const char* GetLibraryPath(void);

  // List of proxy DLLs needed to load the game client
  const char** GetProxyDllPaths(void);

  // Number of proxy DLLs needed to load the game client
  unsigned int GetProxyDllCount(void) const { return m_proxyDllPaths.size(); }

  // Equal to special://profile/addon_data/<parent's id>/system
  const char* GetSystemDirectory(void);

  // Equal to parent's add-on path
  const char* GetContentDirectory(void);

  // Equal to special://profile/addon_data/<parent's id>/save
  const char* GetSaveDirectory(void);

  // Helper functions
  bool AddProxyDll(const std::string& strLibPath);
  bool HasProxyDll(const std::string& strLibPath) const;

  const CGameClient* const  m_parent;
  game_client_properties    m_properties;

  // Buffers to hold the strings
  std::string        m_strLibraryPath;      // immutable
  std::vector<char*> m_proxyDllPaths;       // mutable
  std::string        m_strNetplayServer;    // immutable
  std::string        m_strSystemDirectory;  // immutable
  std::string        m_strContentDirectory; // immutable
  std::string        m_strSaveDirectory;    // immutable
};

} // namespace GAME
