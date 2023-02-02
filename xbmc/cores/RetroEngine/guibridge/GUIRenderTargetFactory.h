/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace RETRO
{
class IRenderManager;
} // namespace RETRO

namespace RETRO_ENGINE
{
class CGUIGameEngineControl;
class CGUIRenderTarget;

class CGUIRenderTargetFactory
{
public:
  CGUIRenderTargetFactory(RETRO::IRenderManager& renderManager);

  /*!
   * \brief Create a render target for a camera view control
   */
  CGUIRenderTarget* CreateRenderControl(CGUIGameEngineControl& gameEngineControl);

private:
  // Construction parameters
  RETRO::IRenderManager& m_renderManager;
};
} // namespace RETRO_ENGINE
} // namespace KODI
