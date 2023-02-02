/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <mutex>

namespace KODI
{
namespace RETRO_ENGINE
{
class CGUIGameEngineControl;
class CGUIRERenderHandle;
class CGUIRERenderTarget;
class CGUIRERenderTargetFactory;

/*!
 * \brief Class to safely route commands between the GUI and the renderer
 *
 * This class is brought up before the GUI and the renderer factory. It provides
 * the GUI with safe access to a registered renderer.
 *
 * Access to the renderer is done through handles. When a handle is no longer
 * needed, it should be destroyed.
 *
 * One kind of handle is provided:
 *
 *   - CGUIRERenderHandle
 *         Allows the holder to invoke render events
 *
 * Each GUI bridge fulfills the following design requirements:
 *
 *   1. No assumption of renderer lifetimes
 *
 *   2. No assumption of GUI element lifetimes, as long as handles are
 *      destroyed before this class is destructed
 *
 *   3. No limit on the number of handles
 */
class CRetroEngineGuiBridge
{
  friend class CGUIRERenderHandle;

public:
  CRetroEngineGuiBridge() = default;
  ~CRetroEngineGuiBridge();

  /*!
   * \brief Register a renderer instance
   *
   * \param factory The interface for creating render targets exposed to the GUI
   */
  void RegisterRenderer(CGUIRERenderTargetFactory& factory);

  /*!
   * \brief Unregister a renderer instance
   */
  void UnregisterRenderer();

  /*!
   * \brief Register a RetroEngine control ("gameengine" skin control)
   *
   * \param control The RetroEngine control
   *
   * \return A handle to invoke render events
   */
  std::shared_ptr<CGUIRERenderHandle> RegisterControl(CGUIGameEngineControl& control);

protected:
  // Functions exposed to friend class CGUIRERenderHandle
  void UnregisterHandle(CGUIRERenderHandle* handle);
  void Render(CGUIRERenderHandle* handle);
  void ClearBackground(CGUIRERenderHandle* handle);
  bool IsDirty(CGUIRERenderHandle* handle);

private:
  /*!
   * \brief Helper function to create or destroy render targets when a
   *        factory is registered/unregistered
   */
  void UpdateRenderTargets();

  /*!
   * \brief Helper function to create a render target
   *
   * \param handle The handle given to the registered GUI element
   *
   * \return A target to receive rendering commands
   */
  CGUIRERenderTarget* CreateRenderTarget(CGUIRERenderHandle* handle);

  // Render events
  CGUIRERenderTargetFactory* m_factory = nullptr;
  std::map<CGUIRERenderHandle*, std::shared_ptr<CGUIRERenderTarget>> m_renderTargets;
  std::mutex m_targetMutex;
};

} // namespace RETRO_ENGINE
} // namespace KODI
