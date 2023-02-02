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
namespace RETRO_ENGINE
{
class CGUIGameEngineControl;
class CRetroEngineGuiBridge;

// --- CGUIRenderHandle ------------------------------------------------------

class CGUIRenderHandle
{
public:
  CGUIRenderHandle(CRetroEngineGuiBridge& renderManager);
  virtual ~CGUIRenderHandle();

  void Render();
  bool IsDirty();
  void ClearBackground();

private:
  // Construction parameters
  CRetroEngineGuiBridge& m_renderManager;
};

// --- CGUIRenderControlHandle -----------------------------------------------

class CGUIRenderControlHandle : public CGUIRenderHandle
{
public:
  CGUIRenderControlHandle(CRetroEngineGuiBridge& renderManager, CGUIGameEngineControl& control);
  ~CGUIRenderControlHandle() override = default;

  CGUIGameEngineControl& GetControl() { return m_control; }

private:
  // Construction parameters
  CGUIGameEngineControl& m_control;
};

} // namespace RETRO_ENGINE
} // namespace KODI
