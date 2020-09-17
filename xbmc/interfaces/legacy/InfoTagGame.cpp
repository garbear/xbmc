/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoTagGame.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

using namespace XBMCAddon;
using namespace xbmc;

InfoTagGame::InfoTagGame()
{
  infoTag = new KODI::GAME::CGameInfoTag();
}

InfoTagGame::InfoTagGame(const KODI::GAME::CGameInfoTag& tag)
{
  infoTag = new KODI::GAME::CGameInfoTag(tag);
}

InfoTagGame::~InfoTagGame()
{
  delete infoTag;
}

String InfoTagGame::getFile()
{
  return infoTag->GetURL();
}

String InfoTagGame::getTitle()
{
  return infoTag->GetTitle();
}

String InfoTagGame::getPlatform()
{
  return infoTag->GetPlatform();
}

String InfoTagGame::getGenres()
{
  return StringUtils::Join(
      infoTag->GetGenres(),
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_gameItemSeparator);
}

String InfoTagGame::getDeveloper()
{
  return infoTag->GetDeveloper();
}

String InfoTagGame::getOverview()
{
  return infoTag->GetOverview();
}

int InfoTagGame::getYear()
{
  return infoTag->GetYear();
}

String InfoTagGame::getID()
{
  return infoTag->GetID();
}

String InfoTagGame::getRegion()
{
  return infoTag->GetRegion();
}

String InfoTagGame::getPublisher()
{
  return infoTag->GetPublisher();
}

String InfoTagGame::getFormat()
{
  return infoTag->GetFormat();
}

String InfoTagGame::getCartridgeType()
{
  return infoTag->GetCartridgeType();
}

String InfoTagGame::getGameClient()
{
  return infoTag->GetGameClient();
}

String InfoTagGame::getCaption()
{
  return infoTag->GetCaption();
}
