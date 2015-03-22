/*
 *      Copyright (C) 2012-2014 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameClientProperties.h"
#include "GameClient.h"
#include "addons/include/kodi_game_types.h"
#include "filesystem/SpecialProtocol.h"

using namespace GAME;

#define GAME_CLIENT_SAVE_DIRECTORY    "save"
#define GAME_CLIENT_SYSTEM_DIRECTORY  "system"

CGameClientProperties::CGameClientProperties(const CGameClient* parent)
  : m_parent(parent)
{
}

const char* CGameClientProperties::GetLibraryPath()
{
  if (m_strLibraryPath.empty())
    m_strLibraryPath = CSpecialProtocol::TranslatePath(m_parent->GameClientPath());

  return m_strLibraryPath.c_str();
}

const char* CGameClientProperties::GetSystemDirectory()
{
  if (m_strSystemDirectory.empty())
    m_strSystemDirectory = CSpecialProtocol::TranslatePath(URIUtils::AddFileToFolder(m_parent->Profile(), GAME_CLIENT_SYSTEM_DIRECTORY));

  return m_strSystemDirectory.c_str();
}

const char* CGameClientProperties::GetContentDirectory()
{
  if (m_strContentDirectory.empty())
    m_strContentDirectory = CSpecialProtocol::TranslatePath(URIUtils::GetParentPath(m_parent->GameClientPath()));

  return m_strContentDirectory.c_str();
}

const char* CGameClientProperties::GetSaveDirectory()
{
  if (m_strSaveDirectory.empty())
    m_strSaveDirectory = CSpecialProtocol::TranslatePath(URIUtils::AddFileToFolder(m_parent->Profile(), GAME_CLIENT_SAVE_DIRECTORY));

  return m_strSaveDirectory.c_str();
}

game_client_properties* CGameClientProperties::CreateProps()
{
  game_client_properties* props = new game_client_properties;
  props->library_path      = GetLibraryPath();
  props->system_directory  = GetSystemDirectory();
  props->content_directory = GetContentDirectory();
  props->save_directory    = GetSaveDirectory();
  return props;
}
