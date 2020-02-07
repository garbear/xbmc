/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SpecialEmulationEngine.h"

using namespace KODI;
using namespace RETRO;

void CSpecialEmulationEngine::PlayFrame(T t, S& state, const R& reward, const P& policy, const A& action)
{
  PlayFrame(t, state, action);
}

void CSpecialEmulationEngine::GetInput(T t, const S& state, const R& reward, const P& policy, A& action)
{
  GetInput(t, state, action);
}

void CSpecialEmulationEngine::PlayFrame(T t, S& state, const A& action)
{
  //! @todo
}

void CSpecialEmulationEngine::GetInput(T t, const S& state, A& action)
{
  //! @todo
}
