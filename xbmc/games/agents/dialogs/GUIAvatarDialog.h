/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "guilib/GUIDialog.h"

#include <memory>

namespace KODI
{
namespace GAME
{
class IAvatarList;

/*!
 * \ingroup games
 */
class CGUIAvatarDialog : public CGUIDialog
{
public:
  CGUIAvatarDialog();
  ~CGUIAvatarDialog() override;

  // Implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  void FrameMove() override;

private:
  // Actions for avatar list
  void UpdateAvatarList();
  void FocusAvatarList();
  bool OnClickAction();

  // Actions for the available buttons
  void ResetAvatars();
  void CloseDialog();

  // GUI parameters
  std::unique_ptr<IAvatarList> m_avatarList;

  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
