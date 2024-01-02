/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IAvatarList.h"
#include "addons/AddonEvents.h"
#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "games/controllers/dialogs/ControllerSelect.h"
#include "games/controllers/types/ControllerTree.h"

#include <map>
#include <memory>
#include <string>

class CFileItemList;
class CGUIViewControl;
class CGUIWindow;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup game
 */
class CGUIAvatarList : public IAvatarList
{
public:
  CGUIAvatarList(CGUIWindow& window);
  ~CGUIAvatarList() override;

  // Implementation of IAvatarList
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  bool Initialize(GameClientPtr gameClient) override;
  void Deinitialize() override;
  bool HasControl(int controlId) override;
  int GetCurrentControl() override;
  void Refresh() override;
  void FrameMove() override;
  void SetFocused() override;
  bool OnSelect() override;
  void ResetAvatars() override;

private:
  // Add-on API
  void OnEvent(const ADDON::AddonEvent& event);

  bool AddItems(const CPortNode& avatar, unsigned int& itemId, const std::string& itemLabel);
  void CleanupItems();
  void OnItemFocus(unsigned int itemIndex);
  void OnItemSelect(unsigned int itemIndex);

  // Controller selection callback
  void OnControllerSelected(const CPortNode& avatar, const ControllerPtr& controller);

  static std::string GetLabel(const CPortNode& avatar);

  // Construction parameters
  CGUIWindow& m_guiWindow;

  // GUI parameters
  CControllerSelect m_controllerSelectDialog;
  std::string m_focusedAvatar; // Address of focused avatar
  int m_currentItem{-1}; // Index of the selected item, or -1 if no item is selected
  std::unique_ptr<CGUIViewControl> m_viewControl;
  std::unique_ptr<CFileItemList> m_vecItems;

  // Game parameters
  GameClientPtr m_gameClient;
  std::map<unsigned int, std::string> m_itemToAddress; // item index -> avatar address
  std::map<std::string, unsigned int> m_addressToItem; // avatar address -> item index
};
} // namespace GAME
} // namespace KODI
