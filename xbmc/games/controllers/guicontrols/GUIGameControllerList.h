/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIControl.h"
#include "guilib/GUIListContainer.h"
#include "peripherals/PeripheralTypes.h"

#include <memory>
#include <string>
#include <vector>

class CScroller;
class TiXmlElement;

namespace KODI
{
namespace GAME
{
class CGUIGameControllerList : public CGUIListContainer
{
public:
  CGUIGameControllerList(int parentID,
                         int controlID,
                         float posX,
                         float posY,
                         float width,
                         float height,
                         ORIENTATION orientation,
                         const CScroller& scroller);
  explicit CGUIGameControllerList(const CGUIGameControllerList& other);

  ~CGUIGameControllerList() override = default;

  // Implementation of CGUIControl via CGUIImage
  CGUIGameControllerList* Clone() const override;
  void UpdateInfo(const CGUIListItem* item) override;

private:
  void UpdatePortIndex(int itemNumber, const std::vector<std::string>& inputPorts);
  void UpdatePortIndex(const PERIPHERALS::PeripheralPtr& agentPeripheral,
                       const std::vector<std::string>& inputPorts);

  unsigned int m_portCount = 0;
  int m_portIndex = -1; // Not connected
};
} // namespace GAME
} // namespace KODI
