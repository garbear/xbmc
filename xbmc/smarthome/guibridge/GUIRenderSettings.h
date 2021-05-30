/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/guibridge/IGUIRenderSettings.h"
#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "threads/CriticalSection.h"
#include "utils/Geometry.h"

namespace KODI
{
namespace SMART_HOME
{
class CGUIGameControl;

class CGUIRenderSettings : public RETRO::IGUIRenderSettings
{
public:
  CGUIRenderSettings();
  CGUIRenderSettings(const CGUIRenderSettings& other);
  ~CGUIRenderSettings() override;

  // implementation of RETRO::IGUIRenderSettings
  RETRO::CRenderSettings GetSettings() const override { return m_renderSettings; }
  CRect GetDimensions() const override;

  // GUI functions
  void SetDimensions(const CRect& dimensions);

private:
  void InitializeRenderSettings();

  // Render parameters
  RETRO::CRenderSettings m_renderSettings;
  CRect m_renderDimensions;

  // Synchronization parameters
  mutable CCriticalSection m_mutex;
};
} // namespace SMART_HOME
} // namespace KODI
