/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IEmulationEngine.h"

namespace KODI
{
namespace RETRO
{
  // clang-format off
  class CSpecialEmulationEngine : public IEmulationEngine
  {
  public:
    virtual ~CSpecialEmulationEngine() = default;

    // Implementation of IEmulationEngine
    void PlayFrame(  T t,       S& state, const R& reward, const P& policy, const A& action ) override;
    void GetReward(  T t, const S& state,       R& reward, const P& policy, const A& action ) override { }
    void Strategize( T t, const S& state, const R& reward,       P& policy, const A& action ) override { }
    void GetInput(   T t, const S& state, const R& reward, const P& policy,       A& action ) override;

  private:
    // Specialized environment functions
    void PlayFrame( T t,       S& state, const A& action );
    void GetInput(  T t, const S& state,       A& action );
  };
  // clang-format on
}
}
