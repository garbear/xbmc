/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderTarget.h"

#include "cores/RetroEngine/guibridge/GUIRenderSettings.h"
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

// --- CGUIRenderTarget --------------------------------------------------------

CGUIRenderTarget::CGUIRenderTarget(RETRO::IRenderManager& renderManager)
  : m_renderManager(renderManager)
{
}

// --- CGUIRenderControl -------------------------------------------------------

CGUIRenderControl::CGUIRenderControl(RETRO::IRenderManager& renderManager,
                                     CGUIGameEngineControl& gameEngineControl)
  : CGUIRenderTarget(renderManager), m_gameEngineControl(gameEngineControl)
{
}

void CGUIRenderControl::Render()
{
  const CRect renderRegion = m_gameEngineControl.GetRenderRegion();
  const RETRO::IGUIRenderSettings& renderSettings = m_gameEngineControl.GetRenderSettings();

  m_renderManager.RenderControl(true, true, renderRegion, &renderSettings);
}
