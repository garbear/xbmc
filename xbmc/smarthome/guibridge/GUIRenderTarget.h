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

// --- CGUIRenderTarget ------------------------------------------------------

/*!
 * \brief A target of rendering commands
 */
class CGUIRenderTarget
{
public:
  CGUIRenderTarget(RETRO::IRenderManager* renderManager);

  virtual ~CGUIRenderTarget() = default;

  /*!
   * \brief Draw the frame to the rendering area
   */
  virtual void Render() = 0;

  /*!
   * \brief Clear the background of the rendering area
   */
  virtual void ClearBackground() {} //! @todo

  /*!
   * \brief Check of the rendering area is dirty
   */
  virtual bool IsDirty() { return true; } //! @todo

protected:
  // Construction parameters
  RETRO::IRenderManager* const m_renderManager;
};

// --- CGUIRenderControl -----------------------------------------------------

/*!
 * \brief A GUI control that is the target of rendering commands
 */
class CGUIRenderControl : public CGUIRenderTarget
{
public:
  CGUIRenderControl(RETRO::IRenderManager* renderManager, RETRO::CGUIGameControl& gameControl);
  ~CGUIRenderControl() override = default;

  // Implementation of CGUIRenderTarget
  void Render() override;

private:
  // Construction parameters
  RETRO::CGUIGameControl& m_gameControl;
};

} // namespace SMART_HOME
} // namespace KODI
