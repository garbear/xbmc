/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameAgent.h"

using namespace KODI;
using namespace GAME;

CGameAgent::CGameAgent()
{
  Initialize();
}

CGameAgent::~CGameAgent()
{
  Deinitialize();
}

void CGameAgent::Initialize()
{
}

void CGameAgent::Deinitialize()
{
}
