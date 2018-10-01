/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class IPlaybackControl
{
public:
  virtual ~IPlaybackControl() = default;

  virtual bool Play() = 0;
  virtual bool Pause() = 0;
  virtual bool Rewind() = 0;
  virtual bool FastForward() = 0;
  virtual bool AnalogRewind() = 0;
  virtual bool AnalogFastForward() = 0;
};
