/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameEngineControl.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/RetroEngine/RetroEngineServices.h"
#include "cores/RetroEngine/guibridge/GUIRenderHandle.h"
#include "cores/RetroEngine/guibridge/GUIRenderSettings.h"
#include "cores/RetroEngine/guibridge/RetroEngineGuiBridge.h"
#include "cores/RetroEngine/guicontrols/GUIGameEngineConfig.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "utils/Geometry.h"

#include <sstream>

using namespace KODI;
using namespace RETRO_ENGINE;

void CGUIGameEngineControl::RenderSettingsDeleter::operator()(CGUIRenderSettings* obj)
{
  delete obj;
};

CGUIGameEngineControl::CGUIGameEngineControl(
    int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_gameEngineConfig(std::make_unique<CGUIGameEngineConfig>()),
    m_renderSettings(new CGUIRenderSettings(*this))
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMEENGINE;

  m_renderSettings->SetDimensions(CRect(CPoint(posX, posY), CSize(width, height)));
}

CGUIGameEngineControl::CGUIGameEngineControl(const CGUIGameEngineControl& other)
  : CGUIControl(other),
    m_gameEngineConfig(std::make_unique<CGUIGameEngineConfig>(*other.m_gameEngineConfig)),
    m_renderSettings(new CGUIRenderSettings(*other.m_renderSettings)),
    m_bHasSavestate(other.m_bHasSavestate),
    m_bHasVideoFilter(other.m_bHasVideoFilter),
    m_bHasStretchMode(other.m_bHasStretchMode),
    m_bHasRotation(other.m_bHasRotation),
    m_savestateInfo(other.m_savestateInfo),
    m_videoFilterInfo(other.m_videoFilterInfo),
    m_stretchModeInfo(other.m_stretchModeInfo),
    m_rotationInfo(other.m_rotationInfo)
{
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(m_width, m_height)));

  SetSavestate(other.m_savestateInfo);
}

CGUIGameEngineControl::~CGUIGameEngineControl()
{
  UnregisterControl();
}

void CGUIGameEngineControl::SetSavestate(const GUILIB::GUIINFO::CGUIInfoLabel& savestatePath)
{
  m_savestateInfo = savestatePath;

  // Check if a savestate path is available without a listitem
  CFileItem empty;
  const std::string strSavestate = m_savestateInfo.GetItemLabel(&empty);
  if (!strSavestate.empty())
    UpdateSavestate(strSavestate);
}

void CGUIGameEngineControl::SetVideoFilter(const GUILIB::GUIINFO::CGUIInfoLabel& videoFilter)
{
  m_videoFilterInfo = videoFilter;

  // Check if a video filter is available without a listitem
  CFileItem empty;
  const std::string strVideoFilter = m_videoFilterInfo.GetItemLabel(&empty);
  if (!strVideoFilter.empty())
    UpdateVideoFilter(strVideoFilter);
}

void CGUIGameEngineControl::SetStretchMode(const GUILIB::GUIINFO::CGUIInfoLabel& stretchMode)
{
  m_stretchModeInfo = stretchMode;

  // Check if a stretch mode is available without a listitem
  CFileItem empty;
  const std::string strStretchMode = m_stretchModeInfo.GetItemLabel(&empty);
  if (!strStretchMode.empty())
    UpdateStretchMode(strStretchMode);
}

void CGUIGameEngineControl::SetRotation(const GUILIB::GUIINFO::CGUIInfoLabel& rotation)
{
  m_rotationInfo = rotation;

  // Check if a rotation is available without a listitem
  CFileItem empty;
  const std::string strRotation = m_rotationInfo.GetItemLabel(&empty);
  if (!strRotation.empty())
    UpdateRotation(strRotation);
}

void CGUIGameEngineControl::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  //! @todo Proper processing which marks when its actually changed
  if (m_renderHandle && m_renderHandle->IsDirty())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIGameEngineControl::Render()
{
  if (m_renderHandle)
    m_renderHandle->Render();

  CGUIControl::Render();
}

bool CGUIGameEngineControl::CanFocus() const
{
  // Unfocusable
  return false;
}

void CGUIGameEngineControl::SetPosition(float posX, float posY)
{
  m_renderSettings->SetDimensions(CRect(CPoint(posX, posY), CSize(m_width, m_height)));

  CGUIControl::SetPosition(posX, posY);
}

void CGUIGameEngineControl::SetWidth(float width)
{
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(width, m_height)));

  CGUIControl::SetWidth(width);
}

void CGUIGameEngineControl::SetHeight(float height)
{
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(m_width, height)));

  CGUIControl::SetHeight(height);
}

void CGUIGameEngineControl::UpdateInfo(const CGUIListItem* item /* = nullptr */)
{
  if (item != nullptr)
  {
    ResetInfo();

    std::string strSavestate = m_savestateInfo.GetItemLabel(item);
    if (!strSavestate.empty())
      UpdateSavestate(strSavestate);

    std::string strVideoFilter = m_videoFilterInfo.GetItemLabel(item);
    if (!strVideoFilter.empty())
      UpdateVideoFilter(strVideoFilter);

    std::string strStretchMode = m_stretchModeInfo.GetItemLabel(item);
    if (!strStretchMode.empty())
      UpdateStretchMode(strStretchMode);

    std::string strRotation = m_rotationInfo.GetItemLabel(item);
    if (!strRotation.empty())
      UpdateRotation(strRotation);
  }
}

void CGUIGameEngineControl::UpdateSavestate(const std::string& savestatePath)
{
  if (savestatePath != m_gameEngineConfig->GetSavestate())
  {
    m_gameEngineConfig->SetSavestate(savestatePath);
    RegisterControl(savestatePath);
    m_bHasSavestate = true;
  }
}

void CGUIGameEngineControl::UpdateVideoFilter(const std::string& strVideoFilter)
{
  m_renderSettings->SetVideoFilter(strVideoFilter);
  m_bHasVideoFilter = true;
}

void CGUIGameEngineControl::UpdateStretchMode(const std::string& strStretchMode)
{
  const RETRO::STRETCHMODE stretchMode =
      RETRO::CRetroPlayerUtils::IdentifierToStretchMode(strStretchMode);
  m_renderSettings->SetStretchMode(stretchMode);
  m_bHasStretchMode = true;
}

void CGUIGameEngineControl::UpdateRotation(const std::string& strRotation)
{
  const unsigned int rotationDegCCW = std::stoul(strRotation);
  m_renderSettings->SetRotationDegCCW(rotationDegCCW);
  m_bHasRotation = true;
}

void CGUIGameEngineControl::ResetInfo()
{
  m_bHasStretchMode = false;
  m_bHasRotation = false;
}

void CGUIGameEngineControl::RegisterControl(const std::string& savestatePath)
{
  UnregisterControl();

  m_renderHandle =
      CServiceBroker::GetRetroEngineServices().GuiBridge(savestatePath).RegisterControl(*this);
}

void CGUIGameEngineControl::UnregisterControl()
{
  m_renderHandle.reset();
}
