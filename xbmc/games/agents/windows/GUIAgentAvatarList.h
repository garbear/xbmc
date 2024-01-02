/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IAgentAvatarList.h"
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
class IAgentAvatarList;

/*!
 * \ingroup games
 */
class CGUIAgentAvatarList : public IAgentAvatarList
{
public:
  CGUIAgentAvatarList(CGUIWindow& window);
  ~CGUIAgentAvatarList() override;

  // Implementation of IAgentAvatarList
  bool Initialize(GameClientPtr gameClient) override;
  void Deinitialize() override;
  void Refresh() override;

private:
  // Add-on API
  void OnEvent(const ADDON::AddonEvent& event);

  /*! @todo
  // GUI helpers
  void InitializeGUI();
  void DeinitializeGUI();
  void AddInputDisabled();
  void AddItems(const PortVec& ports);
  void AddItem(const ControllerPtr& controller, const std::string& controllerAddress);
  void AddPadding();
  */
  void CleanupItems();

  // Construction parameters
  CGUIWindow& m_guiWindow;

  // GUI parameters
  std::unique_ptr<CFileItemList> m_vecItems;
  uint32_t m_alignment{0};

  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
