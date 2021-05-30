/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderTarget.h"

#include "cores/RetroPlayer/guicontrols/GUIGameControl.h"
#include "cores/RetroPlayer/rendering/IRenderManager.h"

using namespace KODI;
using namespace SMART_HOME;

// --- CGUIRenderTarget --------------------------------------------------------

CGUIRenderTarget::CGUIRenderTarget(RETRO::IRenderManager* renderManager)
  : m_renderManager(renderManager)
{
}

// --- CGUIRenderControl -------------------------------------------------------

CGUIRenderControl::CGUIRenderControl(RETRO::IRenderManager* renderManager,
                                     RETRO::CGUIGameControl& gameControl)
  : CGUIRenderTarget(renderManager), m_gameControl(gameControl)
{
}

void CGUIRenderControl::Render()
{
  m_renderManager->RenderControl(true, true, m_gameControl.GetRenderRegion(),
                                 m_gameControl.GetRenderSettings());
}
