/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRERenderTargetFactory.h"

#include "GUIRERenderTarget.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CGUIRERenderTargetFactory::CGUIRERenderTargetFactory(RETRO::IRenderManager& renderManager)
  : m_renderManager(renderManager)
{
}

CGUIRERenderTarget* CGUIRERenderTargetFactory::CreateRenderControl(
    CGUIGameEngineControl& gameEngineControl)
{
  return new CGUIRERenderControl(m_renderManager, gameEngineControl);
}
