/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "listproviders/IListProvider.h"

#include <memory>
#include <stdint.h>
#include <vector>

namespace KODI
{
namespace GAME
{
class CGUIGameControllerProvider : public IListProvider
{
public:
  CGUIGameControllerProvider(unsigned int portCount,
                             int portIndex,
                             uint32_t alignment,
                             int parentID);
  explicit CGUIGameControllerProvider(const CGUIGameControllerProvider& other);

  ~CGUIGameControllerProvider() override;

  // Implementation of IListProvider
  std::unique_ptr<IListProvider> Clone() override;
  bool Update(bool forceRefresh) override;
  void Fetch(std::vector<CGUIListItemPtr>& items) override;
  bool OnClick(const CGUIListItemPtr& item) override { return false; }
  bool OnInfo(const CGUIListItemPtr& item) override { return false; }
  bool OnContextMenu(const CGUIListItemPtr& item) override { return false; }
  void SetDefaultItem(int item, bool always) override {}
  int GetDefaultItem() const override { return -1; }
  bool AlwaysFocusDefaultItem() const override { return false; }

  // Game functions
  ControllerPtr GetControllerProfile() const { return m_controllerProfile; }
  void SetControllerProfile(ControllerPtr controllerProfile);
  unsigned int GetPortCount() const { return m_portCount; }
  int GetPortIndex() const { return m_portIndex; }
  void SetPortCount(unsigned int portCount);
  void SetPortIndex(int portIndex);

private:
  // GUI functions
  void UpdateItems();

  // Game parameters
  ControllerPtr m_controllerProfile;
  unsigned int m_portCount{0};
  int m_portIndex{-1}; // Not connected

  // GUI parameters
  const uint32_t m_alignment;
  std::vector<std::shared_ptr<CGUIListItem>> m_items;
  bool m_bDirty{true};
};
} // namespace GAME
} // namespace KODI
