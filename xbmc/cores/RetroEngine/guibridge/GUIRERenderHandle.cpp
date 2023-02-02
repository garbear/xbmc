/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRERenderHandle.h"

#include "RetroEngineGuiBridge.h"

using namespace KODI;
using namespace RETRO_ENGINE;

// --- CGUIRERenderHandle --------------------------------------------------------

CGUIRERenderHandle::CGUIRERenderHandle(CRetroEngineGuiBridge& renderManager)
  : m_renderManager(renderManager)
{
}

CGUIRERenderHandle::~CGUIRERenderHandle()
{
  m_renderManager.UnregisterHandle(this);
}

void CGUIRERenderHandle::Render()
{
  m_renderManager.Render(this);
}

bool CGUIRERenderHandle::IsDirty()
{
  return m_renderManager.IsDirty(this);
}

void CGUIRERenderHandle::ClearBackground()
{
  m_renderManager.ClearBackground(this);
}

// --- CGUIRERenderControlHandle -------------------------------------------------

CGUIRERenderControlHandle::CGUIRERenderControlHandle(CRetroEngineGuiBridge& renderManager,
                                                 CGUIGameEngineControl& control)
  : CGUIRERenderHandle(renderManager), m_control(control)
{
}
