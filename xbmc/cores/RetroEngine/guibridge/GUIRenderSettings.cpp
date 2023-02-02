/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderSettings.h"

#include "cores/RetroEngine/guicontrols/GUIGameEngineControl.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CGUIRenderSettings::CGUIRenderSettings(CGUIGameEngineControl& guiControl) : m_guiControl(guiControl)
{
  InitializeRenderSettings();
}

CGUIRenderSettings::CGUIRenderSettings(const CGUIRenderSettings& other)
  : m_guiControl(other.m_guiControl), m_renderDimensions(other.m_renderDimensions)
{
  InitializeRenderSettings();
}

CGUIRenderSettings::~CGUIRenderSettings() = default;

void CGUIRenderSettings::InitializeRenderSettings()
{
  m_renderSettings.VideoSettings().SetScalingMethod(RETRO::SCALINGMETHOD::LINEAR);
}

bool CGUIRenderSettings::HasVideoFilter() const
{
  return m_guiControl.HasVideoFilter();
}

bool CGUIRenderSettings::HasStretchMode() const
{
  return m_guiControl.HasStretchMode();
}

bool CGUIRenderSettings::HasRotation() const
{
  return m_guiControl.HasRotation();
}

CRect CGUIRenderSettings::GetDimensions() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_renderDimensions;
}

void CGUIRenderSettings::SetVideoFilter(const std::string& videoFilter)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_renderSettings.VideoSettings().SetVideoFilter(videoFilter);
}

void CGUIRenderSettings::SetStretchMode(RETRO::STRETCHMODE stretchMode)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_renderSettings.VideoSettings().SetRenderStretchMode(stretchMode);
}

void CGUIRenderSettings::SetRotationDegCCW(unsigned int rotationDegCCW)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_renderSettings.VideoSettings().SetRenderRotation(rotationDegCCW);
}

void CGUIRenderSettings::SetDimensions(const CRect& dimensions)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_renderDimensions = dimensions;
}
