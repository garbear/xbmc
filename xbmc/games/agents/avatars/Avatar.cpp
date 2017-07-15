/*
 *  Copyright (C) 2017-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Avatar.h"

#define AVATARS_XML_FILE "avatars.xml"

using namespace KODI;
using namespace GAME;

CAvatar::CAvatar() = default;

bool CAvatar::Deserialize(const TiXmlElement* pElement)
{
  return false; //! @todo
}
