/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IPlaybackControl.h"

class CPlaybackControl : public IPlaybackControl
{
public:
  ~CPlaybackControl() override = default;

  bool Play() override { return false; }
  bool Pause() override { return false; }
  bool Rewind() override { return false; }
  bool FastForward() override { return false; }
  bool AnalogRewind() override { return false; }
  bool AnalogFastForward() override { return false; }
};
