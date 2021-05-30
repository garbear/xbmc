/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIControl.h"

#include <memory>
#include <string>

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{
class CGUIInfoLabel;
}
} // namespace GUILIB

namespace SMART_HOME
{
class CGUICameraConfig;
class CGUIRenderHandle;
class CGUIRenderSettings;

class CGUICameraControl : public CGUIControl
{
public:
  CGUICameraControl(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUICameraControl(const CGUICameraControl& other);
  ~CGUICameraControl() override;

  void Reset();

  // Camera functions
  bool HasPubSubTopic() const;
  const std::string& GetPubSubTopic() const;

  // GUI functions
  void SetPubSubTopic(const std::string& topic);

  // Rendering functions
  const CGUIRenderSettings& GetRenderSettings() const { return *m_renderSettings; }

  // Implementation of CGUIControl
  CGUICameraControl* Clone() const override { return new CGUICameraControl(*this); };
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;
  bool CanFocus() const override;
  void SetPosition(float posX, float posY) override;
  void SetWidth(float width) override;
  void SetHeight(float height) override;

private:
  // Utility class
  struct RenderSettingsDeleter
  {
    // Utility function
    void operator()(CGUIRenderSettings* obj);
  };

  void RegisterControl();
  void UnregisterControl();

  // Camera properties
  std::unique_ptr<CGUICameraConfig> m_cameraConfig;

  // Rendering properties
  std::unique_ptr<CGUIRenderSettings, RenderSettingsDeleter> m_renderSettings;
  std::shared_ptr<CGUIRenderHandle> m_renderHandle;
};

} // namespace SMART_HOME
} // namespace KODI
