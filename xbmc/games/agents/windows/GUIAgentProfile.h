/*
 *  Copyright (C) 2022 Team Kodi
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
class IActivePortList;

//! @todo
class IAgentProfile
{
public:
  virtual ~IAgentProfile() = 0;
};

class CGUIAgentProfile : public CGUIDialog
{
public:
  CGUIAgentProfile();
  ~CGUIAgentProfile() override;

  // Implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  // Button actions
  void CloseDialog();
  void ResetProfile();

  // Actions for port list
  void UpdateActivePortList();

  // GUI parameters
  std::unique_ptr<IActivePortList> m_portList;

  // Game parameters
  GameClientPtr m_gameClient;
  std::string m_agentDid;
  std::unique_ptr<IAgentProfile> m_agentProfile;
};
} // namespace GAME
} // namespace KODI
