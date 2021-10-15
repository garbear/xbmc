/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ControllerSelect.h"
#include "IPortWindow.h"
#include "addons/AddonEvents.h"
#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "games/controllers/types/ControllerTree.h"

#include <map>
#include <set>
#include <string>

class CGUIButtonControl;
class CGUIControlGroupList;
class CGUIWindow;

namespace KODI
{
namespace GAME
{
class CGameClientTopology;
class CGUIPortWindow;

class CGUIPortList : public IPortList
{
public:
  CGUIPortList(CGUIWindow* window, GameClientPtr gameClient);
  ~CGUIPortList() override { Deinitialize(); }

  // Implementation of IPortList
  bool Initialize() override;
  void Deinitialize() override;
  void Refresh() override;
  void OnFocus(unsigned int buttonIndex) override;
  void OnSelect(unsigned int buttonIndex) override;
  void ResetPorts() override;

private:
  // Add-on API
  void OnEvent(const ADDON::AddonEvent& event);

  void LoadPorts();
  bool AddButtons(const CPortNode& port, unsigned int& buttonId, const std::string& buttonLabel);
  void CleanupButtons();
  void OnControllerSelected(CPortNode& port, ControllerPtr controller);
  void ActivateController(CPortNode& port, unsigned int controllerIndex);
  void DeactivateController(CPortNode& port);

  // GUI stuff
  CGUIWindow* const m_guiWindow;
  CGUIControlGroupList* m_portList = nullptr;
  CGUIButtonControl* m_portButtonTemplate = nullptr;
  CControllerSelect m_controllerSelectDialog;
  std::string m_focusedPort; // Address of focused port

  // Game stuff
  GameClientPtr m_gameClient;
  CControllerTree m_controllerTree;
  std::map<unsigned int, std::string> m_buttonToAddress; // button ID -> port address
  std::map<std::string, unsigned int> m_addressToButton; // port address -> button ID
};
} // namespace GAME
} // namespace KODI
