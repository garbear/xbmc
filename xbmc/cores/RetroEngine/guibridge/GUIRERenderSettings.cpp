/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRERenderSettings.h"

#include "cores/RetroEngine/guicontrols/GUIGameEngineControl.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CGUIRERenderSettings::CGUIRERenderSettings(CGUIGameEngineControl& guiControl) : m_guiControl(guiControl)
{
  InitializeRenderSettings();
}

CGUIRERenderSettings::CGUIRERenderSettings(const CGUIRERenderSettings& other)
  : m_guiControl(other.m_guiControl), m_renderDimensions(other.m_renderDimensions)
{
  InitializeRenderSettings();
}

CGUIRERenderSettings::~CGUIRERenderSettings() = default;

void CGUIRERenderSettings::InitializeRenderSettings()
{
  m_renderSettings.VideoSettings().SetScalingMethod(RETRO::SCALINGMETHOD::LINEAR);
}

bool CGUIRERenderSettings::HasVideoFilter() const
{
  return m_guiControl.HasVideoFilter();
}

bool CGUIRERenderSettings::HasStretchMode() const
{
  return m_guiControl.HasStretchMode();
}

bool CGUIRERenderSettings::HasRotation() const
{
  return m_guiControl.HasRotation();
}

CRect CGUIRERenderSettings::GetDimensions() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_renderDimensions;
}

void CGUIRERenderSettings::SetVideoFilter(const std::string& videoFilter)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_renderSettings.VideoSettings().SetVideoFilter(videoFilter);
}

void CGUIRERenderSettings::SetStretchMode(RETRO::STRETCHMODE stretchMode)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_renderSettings.VideoSettings().SetRenderStretchMode(stretchMode);
}

void CGUIRERenderSettings::SetRotationDegCCW(unsigned int rotationDegCCW)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_renderSettings.VideoSettings().SetRenderRotation(rotationDegCCW);
}

void CGUIRERenderSettings::SetDimensions(const CRect& dimensions)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_renderDimensions = dimensions;
}
