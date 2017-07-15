/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IActivePortList.h"
#include "addons/AddonEvents.h"
#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "games/ports/types/PortNode.h"
#include "utils/Observer.h"

#include <memory>

class CFileItemList;
class CGUIWindow;

namespace KODI
{
namespace GAME
{
class CController;
class IActivePortList;

class CGUIActivePortList : public IActivePortList, public Observer
{
public:
  CGUIActivePortList(CGUIWindow& window);
  ~CGUIActivePortList() override;

  // Implementation of IActivePortList
  bool Initialize(GameClientPtr gameClient) override;
  void Deinitialize() override;
  void Refresh() override;
  ControllerVector GetControllers() override { return m_controllers; }

  // Implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

private:
  // Add-on API
  void OnEvent(const ADDON::AddonEvent& event);

  void AddInputDisabled();
  void AddItems(const PortVec& ports);
  void AddItem(ControllerPtr controller);
  void CleanupItems();

  // Construction parameters
  CGUIWindow& m_guiWindow;

  // GUI parameters
  std::unique_ptr<CFileItemList> m_vecItems;

  // Game parameters
  GameClientPtr m_gameClient;
  ControllerVector m_controllers;
};
} // namespace GAME
} // namespace KODI
