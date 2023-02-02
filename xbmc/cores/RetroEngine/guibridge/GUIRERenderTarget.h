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
}

namespace RETRO_ENGINE
{
class CGUIGameEngineControl;
class IRenderManager;

// --- CGUIRERenderTarget ------------------------------------------------------

/*!
 * \brief A target of rendering commands
 */
class CGUIRERenderTarget
{
public:
  CGUIRERenderTarget(RETRO::IRenderManager& renderManager);

  virtual ~CGUIRERenderTarget() = default;

  /*!
   * \brief Draw the frame to the rendering area
   */
  virtual void Render() = 0;

  /*!
   * \brief Clear the background of the rendering area
   */
  virtual void ClearBackground() = 0;

  /*!
   * \brief Check of the rendering area is dirty
   */
  virtual bool IsDirty() = 0;

protected:
  // Construction parameters
  RETRO::IRenderManager& m_renderManager;
};

// --- CGUIRERenderControl -----------------------------------------------------

/*!
 * \brief A GUI control that is the target of rendering commands
 */
class CGUIRERenderControl : public CGUIRERenderTarget
{
public:
  CGUIRERenderControl(RETRO::IRenderManager& renderManager, CGUIGameEngineControl& gameEngineControl);
  ~CGUIRERenderControl() override = default;

  // Implementation of CGUIRERenderTarget
  void Render() override;
  void ClearBackground() override {} //! @todo
  bool IsDirty() override { return true; } //! @todo

private:
  // Construction parameters
  CGUIGameEngineControl& m_gameEngineControl;
};

} // namespace RETRO_ENGINE
} // namespace KODI
