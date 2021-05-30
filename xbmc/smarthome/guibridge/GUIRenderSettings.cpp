/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderSettings.h"

#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"

using namespace KODI;
using namespace SMART_HOME;

CGUIRenderSettings::CGUIRenderSettings()
{
  InitializeRenderSettings();
}

CGUIRenderSettings::CGUIRenderSettings(const CGUIRenderSettings& other)
  : m_renderDimensions(other.m_renderDimensions)
{
  InitializeRenderSettings();
}

CGUIRenderSettings::~CGUIRenderSettings() = default;

void CGUIRenderSettings::InitializeRenderSettings()
{
  m_renderSettings.VideoSettings().SetScalingMethod(RETRO::SCALINGMETHOD::LINEAR);
}

CRect CGUIRenderSettings::GetDimensions() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_renderDimensions;
}

void CGUIRenderSettings::SetDimensions(const CRect& dimensions)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_renderDimensions = dimensions;
}
