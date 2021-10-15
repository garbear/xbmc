/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/RepositoryUpdater.h"
#include "games/GameTypes.h"
#include "guilib/GUIDialog.h"

#include <memory>

namespace KODI
{
namespace GAME
{
class IControllerList;
class IPortList;

class CGUIPortWindow : public CGUIDialog
{
public:
  CGUIPortWindow();
  ~CGUIPortWindow() override;

  // implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

protected:
  // implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  void OnPortFocused(unsigned int buttonIndex);
  void OnPortSelected(unsigned int buttonIndex);
  void UpdatePorts();

  // Action for the available button
  void ResetPorts();

  std::unique_ptr<IPortList> m_portList;

  // Game paremeters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
