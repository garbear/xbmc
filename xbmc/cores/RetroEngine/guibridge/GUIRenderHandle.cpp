/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderHandle.h"

#include "RetroEngineGuiBridge.h"

using namespace KODI;
using namespace RETRO_ENGINE;

// --- CGUIRenderHandle --------------------------------------------------------

CGUIRenderHandle::CGUIRenderHandle(CRetroEngineGuiBridge& renderManager)
  : m_renderManager(renderManager)
{
}

CGUIRenderHandle::~CGUIRenderHandle()
{
  m_renderManager.UnregisterHandle(this);
}

void CGUIRenderHandle::Render()
{
  m_renderManager.Render(this);
}

bool CGUIRenderHandle::IsDirty()
{
  return m_renderManager.IsDirty(this);
}

void CGUIRenderHandle::ClearBackground()
{
  m_renderManager.ClearBackground(this);
}

// --- CGUIRenderControlHandle -------------------------------------------------

CGUIRenderControlHandle::CGUIRenderControlHandle(CRetroEngineGuiBridge& renderManager,
                                                 CGUIGameEngineControl& control)
  : CGUIRenderHandle(renderManager), m_control(control)
{
}
