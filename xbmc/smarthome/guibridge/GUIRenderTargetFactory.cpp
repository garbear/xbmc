/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderTargetFactory.h"

#include "GUIRenderTarget.h"

using namespace KODI;
using namespace SMART_HOME;

CGUIRenderTargetFactory::CGUIRenderTargetFactory(RETRO::IRenderManager* renderManager)
  : m_renderManager(renderManager)
{
}

CGUIRenderTarget* CGUIRenderTargetFactory::CreateRenderControl(RETRO::CGUIGameControl& gameControl)
{
  return new CGUIRenderControl(m_renderManager, gameControl);
}
