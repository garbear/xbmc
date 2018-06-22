/*
 *  Copyright (C) 2018-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <memory>

namespace KODI
{
namespace RETRO
{
class CGUIGamePlaybackHandle;
}

namespace GAME
{
class CDialogGameRewind : public CGUIDialog
{
public:
  CDialogGameRewind();
  ~CDialogGameRewind() override = default;

  // implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

private:
  bool RegisterDialog();
  void UnregisterDialog();

  std::shared_ptr<RETRO::CGUIGamePlaybackHandle> m_gamePlaybackHandle;
};
} // namespace GAME
} // namespace KODI
