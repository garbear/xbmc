/*
 *  Copyright (C) 2022 Team Kodi
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
#include "guilib/GUIListItem.h"
#include "utils/Variant.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

CGUIGameControllerProvider::CGUIGameControllerProvider(unsigned int portCount,
                                                       int portIndex,
                                                       int parentID)
  : IListProvider(parentID), m_portIndex(portIndex)
{
  SetPortCount(portCount);
}

CGUIGameControllerProvider::CGUIGameControllerProvider(const CGUIGameControllerProvider& other)
  : IListProvider(other.m_parentID), m_portIndex(other.m_portIndex)
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
  if (controllerProfile != m_controllerProfile)
  {
    m_controllerProfile = std::move(controllerProfile);
    UpdateItems();
  }
}

void CGUIGameControllerProvider::SetPortCount(unsigned int portCount)
{
  if (m_portCount != portCount)
  {
    m_portCount = portCount;
    m_itemCount = portCount + 1;

    // Create or destroy GUI items
    while (m_items.size() < static_cast<size_t>(m_itemCount))
      m_items.emplace_back(std::make_shared<CGUIListItem>());
    if (m_items.size() > static_cast<size_t>(m_itemCount))
      m_items.resize(std::max(m_itemCount, 0u));

    UpdateItems();
  }
}

void CGUIGameControllerProvider::SetPortIndex(int portIndex)
{
  if (m_portIndex != portIndex)
  {
    m_portIndex = portIndex;

    UpdateItems();
  }
}

void CGUIGameControllerProvider::UpdateItems()
{
  ControllerPtr controller = m_controllerProfile;

  if (!controller)
  {
    // Fall back to default controller
    GAME::CGameServices& gameServices = CServiceBroker::GetGameServices();
    controller = gameServices.GetDefaultController();
  }

  int portIndex = -1;
  for (const CGUIListItemPtr& item : m_items)
  {
    if (portIndex == m_portIndex && controller)
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

    ++portIndex;
  }

  m_bDirty = true;
}
