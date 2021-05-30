/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUICameraControl.h"

#include "ServiceBroker.h"
#include "guilib/GUIControl.h" //! @todo For IDE
#include "smarthome/SmartHomeServices.h"
#include "smarthome/guibridge/GUIRenderHandle.h"
#include "smarthome/guibridge/GUIRenderSettings.h"
#include "smarthome/guibridge/SmartHomeGuiBridge.h"
#include "smarthome/guicontrols/GUICameraConfig.h"
#include "smarthome/ros2/Ros2.h"
#include "utils/Geometry.h"

#include <sstream>

using namespace KODI;
using namespace SMART_HOME;

void CGUICameraControl::RenderSettingsDeleter::operator()(CGUIRenderSettings* obj)
{
  delete obj;
};

CGUICameraControl::CGUICameraControl(
    int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_cameraConfig(std::make_unique<CGUICameraConfig>()),
    m_renderSettings(new CGUIRenderSettings)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_CAMERA;

  m_renderSettings->SetDimensions(CRect(CPoint(posX, posY), CSize(width, height)));
}

CGUICameraControl::CGUICameraControl(const CGUICameraControl& other)
  : CGUIControl(other),
    m_cameraConfig(std::make_unique<CGUICameraConfig>(*other.m_cameraConfig)),
    m_renderSettings(new CGUIRenderSettings(*other.m_renderSettings))
{
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(m_width, m_height)));
}

CGUICameraControl::~CGUICameraControl()
{
  Reset();
}

void CGUICameraControl::Reset()
{
  UnregisterControl();
  m_cameraConfig->Reset();
}

bool CGUICameraControl::HasPubSubTopic() const
{
  return !m_cameraConfig->GetTopic().empty();
}

const std::string& CGUICameraControl::GetPubSubTopic() const
{
  return m_cameraConfig->GetTopic();
}

void CGUICameraControl::SetPubSubTopic(const std::string& topic)
{
  return m_cameraConfig->SetTopic(topic);
}

void CGUICameraControl::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  // Register control with GUI bridge if we haven't already done so
  if (!m_renderHandle && HasPubSubTopic())
    RegisterControl();

  //! @todo Proper processing which marks when its actually changed
  if (m_renderHandle && m_renderHandle->IsDirty())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUICameraControl::Render()
{
  if (m_renderHandle)
    m_renderHandle->Render();

  CGUIControl::Render();
}

bool CGUICameraControl::CanFocus() const
{
  // Unfocusable
  return false;
}

void CGUICameraControl::SetPosition(float posX, float posY)
{
  m_renderSettings->SetDimensions(CRect(CPoint(posX, posY), CSize(m_width, m_height)));

  CGUIControl::SetPosition(posX, posY);
}

void CGUICameraControl::SetWidth(float width)
{
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(width, m_height)));

  CGUIControl::SetWidth(width);
}

void CGUICameraControl::SetHeight(float height)
{
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(m_width, height)));

  CGUIControl::SetHeight(height);
}

void CGUICameraControl::RegisterControl()
{
  const std::string& topic = GetPubSubTopic();

  //! @todo
  CServiceBroker::GetSmartHomeServices().Ros2().RegisterImageTopic(topic);
  m_renderHandle = CServiceBroker::GetSmartHomeServices().GuiBridge(topic).RegisterControl(*this);
}

void CGUICameraControl::UnregisterControl()
{
  m_renderHandle.reset();
}
