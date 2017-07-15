/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameControllerProvider.h"

#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/ports/windows/GUIPortDefines.h"
#include "guilib/GUIFont.h"
#include "guilib/GUIListItem.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

namespace
{
// Prepend a "disconnected" icon
constexpr unsigned int ITEM_COUNT = MAX_PORT_COUNT + 1;
} // namespace

CGUIGameControllerProvider::CGUIGameControllerProvider(unsigned int portCount,
                                                       int portIndex,
                                                       uint32_t alignment,
                                                       int parentID)
  : IListProvider(parentID), m_portIndex(portIndex), m_alignment(alignment)
{
  SetPortCount(portCount);
}

CGUIGameControllerProvider::CGUIGameControllerProvider(const CGUIGameControllerProvider& other)
  : IListProvider(other.m_parentID), m_portIndex(other.m_portIndex), m_alignment(other.m_alignment)
{
  SetPortCount(other.m_portCount);
}

CGUIGameControllerProvider::~CGUIGameControllerProvider() = default;

std::unique_ptr<IListProvider> CGUIGameControllerProvider::Clone()
{
  return std::make_unique<CGUIGameControllerProvider>(*this);
}

bool CGUIGameControllerProvider::Update(bool forceRefresh)
{
  bool bDirty = false;
  std::swap(bDirty, m_bDirty);
  return bDirty;
}

void CGUIGameControllerProvider::Fetch(std::vector<CGUIListItemPtr>& items)
{
  items = m_items;
}

void CGUIGameControllerProvider::SetControllerProfile(ControllerPtr controllerProfile)
{
  const std::string oldControllerId = m_controllerProfile ? m_controllerProfile->ID() : "";
  const std::string newControllerId = controllerProfile ? controllerProfile->ID() : "";

  if (oldControllerId != newControllerId)
  {
    m_controllerProfile = std::move(controllerProfile);
    UpdateItems();
  }
}

void CGUIGameControllerProvider::SetPortCount(unsigned int portCount)
{
  // Validate parameters
  if (portCount > MAX_PORT_COUNT)
    portCount = MAX_PORT_COUNT;

  // Initialize items
  if (m_items.empty())
  {
    for (unsigned int item = 0; item < ITEM_COUNT; ++item)
      m_items.emplace_back(std::make_shared<CGUIListItem>());
  }

  // Update items
  if (m_portCount != portCount)
  {
    m_portCount = portCount;
    UpdateItems();
  }
}

void CGUIGameControllerProvider::SetPortIndex(int portIndex)
{
  // Update items
  if (m_portIndex != portIndex)
  {
    m_portIndex = portIndex;
    UpdateItems();
  }
}

void CGUIGameControllerProvider::UpdateItems()
{
  ControllerPtr controller = m_controllerProfile;

  int portIndex = -1;
  unsigned int i = 0;
  for (const CGUIListItemPtr& item : m_items)
  {
    // Add padding if aligning to the right
    if (m_alignment == XBFONT_RIGHT && i++ < MAX_PORT_COUNT - m_portCount)
    {
      item->SetLabel("");
      item->ClearProperties();
      item->ClearArt();
      continue;
    }

    if (portIndex++ == m_portIndex && controller)
    {
      item->SetLabel(controller->Layout().Label());
      item->SetProperty("Addon.ID", controller->ID());
      item->SetArt("icon", controller->Layout().ImagePath());
    }
    else
    {
      item->SetLabel("");
      item->ClearProperties();
      item->ClearArt();
    }
  }

  m_bDirty = true;
}
