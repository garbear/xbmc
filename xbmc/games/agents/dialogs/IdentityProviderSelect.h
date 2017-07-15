/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

namespace KODI
{
namespace GAME
{
class CIdentityProviderSelect : public CThread
{
public:
  CIdentityProviderSelect();
  ~CIdentityProviderSelect() override;

  void Initialize();
  void Deinitialize();

protected:
  // Implementation of CThread
  void Process() override;
};
} // namespace GAME
} // namespace KODI
