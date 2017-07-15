/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameController.h"

#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "guilib/GUIListItem.h"
#include "utils/log.h"

#include <mutex>

using namespace KODI;
using namespace GAME;

CGUIGameController::CGUIGameController(int parentID,
                                       int controlID,
                                       float posX,
                                       float posY,
                                       float width,
                                       float height,
                                       const CTextureInfo& texture)
  : CGUIImage(parentID, controlID, posX, posY, width, height, texture)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLER;
}

CGUIGameController::CGUIGameController(const CGUIGameController& from) : CGUIImage(from)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLER;
}

CGUIGameController* CGUIGameController::Clone(void) const
{
  return new CGUIGameController(*this);
}

void CGUIGameController::Render(void)
{
  CGUIImage::Render();

  std::unique_lock<CCriticalSection> lock(m_mutex);

  if (m_currentController)
  {
    //! @todo Render pressed buttons
  }
}

void CGUIGameController::UpdateInfo(const CGUIListItem* item /* = nullptr */)
{
  CGUIImage::UpdateInfo(item);

  if (item != nullptr)
  {
    const std::string controllerId = item->GetProperty("Addon.ID").asString();
    if (!controllerId.empty())
      ActivateController(controllerId);
  }
}

void CGUIGameController::ActivateController(const std::string& controllerId)
{
  CGameServices& gameServices = CServiceBroker::GetGameServices();

  ControllerPtr controller = gameServices.GetController(controllerId);

  ActivateController(controller);
}

void CGUIGameController::ActivateController(const ControllerPtr& controller)
{
  if (controller)
  {
    std::unique_lock<CCriticalSection> lock(m_mutex);

    if (!m_currentController || controller->ID() != m_currentController->ID())
    {
      m_currentController = controller;

      lock.unlock();

      //! @todo In the past this sometimes failed on window init
      SetFileName(m_currentController->Layout().ImagePath());
    }
  }
}
