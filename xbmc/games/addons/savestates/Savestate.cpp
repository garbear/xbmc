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

#include "Savestate.h"
#include "SavestateDefines.h"
#include "utils/Variant.h"

using namespace GAME;

void CSavestate::Reset()
{
  m_path.clear();
  m_type = SAVETYPE::UNKNOWN;
  m_slot = -1;
  m_label.clear();
  m_size = 0;
  m_gameClient.clear();
  m_databaseId = -1;
  m_gamePath.clear();
  m_gameCRC.clear();
  m_playtimeFrames = 0;
  m_playtimeWallClock = 0.0;
  m_timestamp.Reset();
  m_thumbnail.clear();
}

void CSavestate::Serialize(CVariant& value) const
{
  value[SAVESTATE_FIELD_PATH] = m_path;
  value[SAVESTATE_FIELD_TYPE] = static_cast<unsigned int>(m_type);
  value[SAVESTATE_FIELD_SLOT] = m_slot;
  value[SAVESTATE_FIELD_LABEL] = m_label;
  value[SAVESTATE_FIELD_SIZE] = static_cast<unsigned int>(m_size);
  value[SAVESTATE_FIELD_GAMECLIENT] = m_gameClient;
  value[SAVESTATE_FIELD_DB_ID] = m_databaseId;
  value[SAVESTATE_FIELD_GAME_PATH] = m_gamePath;
  value[SAVESTATE_FIELD_GAME_CRC] = m_gameCRC;
  value[SAVESTATE_FIELD_FRAMES] = m_playtimeFrames;
  value[SAVESTATE_FIELD_WALLCLOCK] = m_playtimeWallClock;
  value[SAVESTATE_FIELD_TIMESTAMP] = m_timestamp.GetAsDBDateTime();
  value[SAVESTATE_FIELD_THUMBNAIL] = m_thumbnail;
}

void CSavestate::Deserialize(const CVariant& value)
{
  m_path = value[SAVESTATE_FIELD_PATH].asString();
  m_type = static_cast<SAVETYPE>(value[SAVESTATE_FIELD_TYPE].asInteger());
  m_slot = static_cast<int>(value[SAVESTATE_FIELD_SLOT].asInteger());
  m_label = value[SAVESTATE_FIELD_LABEL].asString();
  m_size = static_cast<size_t>(value[SAVESTATE_FIELD_SIZE].asUnsignedInteger());
  m_gameClient = value[SAVESTATE_FIELD_GAMECLIENT].asString();
  m_databaseId = static_cast<int>(value[SAVESTATE_FIELD_DB_ID].asInteger());
  m_gamePath = value[SAVESTATE_FIELD_GAME_PATH].asString();
  m_gameCRC = value[SAVESTATE_FIELD_GAME_CRC].asString();
  m_playtimeFrames = value[SAVESTATE_FIELD_FRAMES].asUnsignedInteger();
  m_playtimeWallClock = value[SAVESTATE_FIELD_WALLCLOCK].asDouble();
  m_timestamp.FromDBDateTime(value[SAVESTATE_FIELD_TIMESTAMP].asString());
  m_thumbnail = value[SAVESTATE_FIELD_THUMBNAIL].asString();
}
