/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIControl.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

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

namespace RETRO_ENGINE
{
class CGUIGameEngineConfig;
class CGUIRenderHandle;
class CGUIRenderSettings;

class CGUIGameEngineControl : public CGUIControl
{
public:
  CGUIGameEngineControl(
      int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIGameEngineControl(const CGUIGameEngineControl& other);
  ~CGUIGameEngineControl() override;

  // GUI functions
  void SetSavestate(const GUILIB::GUIINFO::CGUIInfoLabel& savestatePath);
  void SetVideoFilter(const GUILIB::GUIINFO::CGUIInfoLabel& videoFilter);
  void SetStretchMode(const GUILIB::GUIINFO::CGUIInfoLabel& stretchMode);
  void SetRotation(const GUILIB::GUIINFO::CGUIInfoLabel& rotation);

  // Rendering functions
  bool HasSavestate() const { return m_bHasSavestate; }
  bool HasVideoFilter() const { return m_bHasVideoFilter; }
  bool HasStretchMode() const { return m_bHasStretchMode; }
  bool HasRotation() const { return m_bHasRotation; }
  const CGUIRenderSettings& GetRenderSettings() const { return *m_renderSettings; }

  // Implementation of CGUIControl
  CGUIGameEngineControl* Clone() const override { return new CGUIGameEngineControl(*this); };
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;
  bool CanFocus() const override;
  void SetPosition(float posX, float posY) override;
  void SetWidth(float width) override;
  void SetHeight(float height) override;
  void UpdateInfo(const CGUIListItem* item = nullptr) override;

private:
  // Utility class
  struct RenderSettingsDeleter
  {
    // Utility function
    void operator()(CGUIRenderSettings* obj);
  };

  void UpdateSavestate(const std::string& strSavestatePath);
  void UpdateVideoFilter(const std::string& strVideoFilter);
  void UpdateStretchMode(const std::string& strStretchMode);
  void UpdateRotation(const std::string& strRotation);
  void ResetInfo();

  void RegisterControl(const std::string& savestatePath);
  void UnregisterControl();

  // Camera properties
  std::unique_ptr<CGUIGameEngineConfig> m_gameEngineConfig;

  // Rendering properties
  std::unique_ptr<CGUIRenderSettings, RenderSettingsDeleter> m_renderSettings;
  std::shared_ptr<CGUIRenderHandle> m_renderHandle;
  bool m_bHasSavestate{false};
  bool m_bHasVideoFilter{false};
  bool m_bHasStretchMode{false};
  bool m_bHasRotation{false};

  // GUI properties
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_savestateInfo;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_videoFilterInfo;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_stretchModeInfo;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_rotationInfo;
};

} // namespace RETRO_ENGINE
} // namespace KODI
