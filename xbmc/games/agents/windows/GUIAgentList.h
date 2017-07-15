/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IAgentList.h"
#include "addons/AddonEvents.h"
#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "games/controllers/types/ControllerTree.h"
#include "utils/Observer.h"

#include <map>
#include <set>
#include <string>

class CFileItemList;
class CGUIViewControl;
class CGUIWindow;

namespace KODI
{
namespace GAME
{
class CGameAgent;

class CGUIAgentList : public IAgentList, public Observer
{
public:
  CGUIAgentList(CGUIWindow& window);
  ~CGUIAgentList() override;

  // Implementation of IAgentList
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  bool Initialize(GameClientPtr gameClient) override;
  void Deinitialize() override;
  bool HasControl(int controlId) const override;
  int GetCurrentControl() const override;
  void Refresh() override;
  void SetFocused() override;

  // Implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

private:
  // Add-on API
  void OnEvent(const ADDON::AddonEvent& event);

  void AddItem(const CGameAgent& agent);
  void CleanupItems();

  // Construction parameters
  CGUIWindow& m_guiWindow;

  // GUI parameters
  int m_currentItem{-1}; // Index of the selected item, or -1 if no item is selected
  std::unique_ptr<CGUIViewControl> m_viewControl;
  std::unique_ptr<CFileItemList> m_vecItems;

  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
