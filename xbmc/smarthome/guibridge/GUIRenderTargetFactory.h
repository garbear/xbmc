/*
 *  Copyright (C) 2021 Team Kodi
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
class CGUIGameControl;
class IRenderManager;
} // namespace RETRO
namespace SMART_HOME
{
class CGUIRenderTarget;

class CGUIRenderTargetFactory
{
public:
  CGUIRenderTargetFactory(RETRO::IRenderManager* renderManager);

  /*!
   * \brief Create a render target for a game control
   *
   * TODO: Break game dependency
   */
  CGUIRenderTarget* CreateRenderControl(RETRO::CGUIGameControl& gameControl);

private:
  // Construction parameters
  RETRO::IRenderManager* m_renderManager;
};
} // namespace SMART_HOME
} // namespace KODI
