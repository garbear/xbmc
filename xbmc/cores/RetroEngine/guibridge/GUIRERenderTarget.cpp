/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRERenderTarget.h"

#include "cores/RetroEngine/guibridge/GUIRERenderSettings.h"
#include "cores/RetroEngine/guicontrols/GUIGameEngineControl.h"
#include "cores/RetroPlayer/rendering/IRenderManager.h"
#include "utils/Geometry.h" //! @todo For IDE

using namespace KODI;
using namespace RETRO_ENGINE;

namespace KODI
{
namespace RETRO
{
class IGUIRenderSettings;
}
} // namespace KODI

// --- CGUIRERenderTarget --------------------------------------------------------

CGUIRERenderTarget::CGUIRERenderTarget(RETRO::IRenderManager& renderManager)
  : m_renderManager(renderManager)
{
}

// --- CGUIRERenderControl -------------------------------------------------------

CGUIRERenderControl::CGUIRERenderControl(RETRO::IRenderManager& renderManager,
                                     CGUIGameEngineControl& gameEngineControl)
  : CGUIRERenderTarget(renderManager), m_gameEngineControl(gameEngineControl)
{
}

void CGUIRERenderControl::Render()
{
  const CRect renderRegion = m_gameEngineControl.GetRenderRegion();
  const RETRO::IGUIRenderSettings& renderSettings = m_gameEngineControl.GetRenderSettings();

  m_renderManager.RenderControl(true, true, renderRegion, &renderSettings);
}
