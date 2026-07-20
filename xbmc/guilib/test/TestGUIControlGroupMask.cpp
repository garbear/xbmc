/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "guilib/GUIControlGroupMask.h"
#include "guilib/GUIListItem.h"
#include "rendering/RenderSystem.h"
#include "windowing/WinSystem.h"

#include <gtest/gtest.h>

namespace
{
class CTestRenderSystem : public CRenderSystemBase
{
public:
  bool InitRenderSystem() override { return true; }
  bool DestroyRenderSystem() override { return true; }
  bool ResetRenderSystem(int, int) override { return true; }
  bool BeginRender() override { return true; }
  bool EndRender() override { return true; }
  void PresentRender(bool, bool) override {}
  bool ClearBuffers(KODI::UTILS::COLOR::Color) override { return true; }
  bool IsExtSupported(const char*) const override { return false; }
  void SetViewPort(const CRect&) override {}
  void GetViewPort(CRect&) override {}
  void SetScissors(const CRect&) override {}
  void ResetScissors() override {}
  void CaptureStateBlock() override {}
  void ApplyStateBlock() override {}
  void SetCameraPosition(const CPoint&, int, int, float) override {}
};

class CTestWinSystem : public CWinSystemBase
{
public:
  CRenderSystemBase* GetRenderSystem() override { return &m_renderSystem; }
  bool CreateNewWindow(const std::string&, bool, RESOLUTION_INFO&) override { return false; }
  bool ResizeWindow(int, int, int, int) override { return false; }
  bool SetFullScreen(bool, RESOLUTION_INFO&, bool) override { return false; }
  void Register(IDispResource*) override {}
  void Unregister(IDispResource*) override {}

private:
  CTestRenderSystem m_renderSystem;
};

class TestGUIControlGroupMask : public testing::Test
{
protected:
  void SetUp() override { CServiceBroker::RegisterWinSystem(&m_winSystem); }
  void TearDown() override { CServiceBroker::UnregisterWinSystem(); }

private:
  CTestWinSystem m_winSystem;
};

class CTestItemControl : public CGUIControl
{
public:
  CTestItemControl() : CGUIControl(0, 0, 0.0f, 0.0f, 1.0f, 1.0f) {}
  CTestItemControl* Clone() const override { return new CTestItemControl(*this); }

  void UpdateInfo(const CGUIListItem* item) override { infoItem = item; }
  void UpdateVisibility(const CGUIListItem* item) override
  {
    visibilityItem = item;
    CGUIControl::UpdateVisibility(item);
  }

  const CGUIListItem* infoItem{nullptr};
  const CGUIListItem* visibilityItem{nullptr};
};
} // namespace

TEST_F(TestGUIControlGroupMask, UpdateInfoPropagatesItem)
{
  CGUIControlGroupMask group(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
  auto* child = new CTestItemControl;
  group.AddControl(child);
  CGUIListItem item;

  group.UpdateInfo(&item);

  EXPECT_EQ(&item, child->infoItem);
  EXPECT_EQ(&item, child->visibilityItem);
}

TEST_F(TestGUIControlGroupMask, ProcessPropagatesAndClearsItem)
{
  CGUIControlGroupMask group(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
  auto* child = new CTestItemControl;
  group.AddControl(child);
  CGUIListItem item;
  CDirtyRegionList dirtyRegions;

  group.UpdateVisibility(&item);
  group.Process(0, dirtyRegions);
  EXPECT_EQ(&item, child->visibilityItem);

  group.Process(0, dirtyRegions);
  EXPECT_EQ(nullptr, child->visibilityItem);
}

TEST_F(TestGUIControlGroupMask, NestedGroupMaskPropagatesItem)
{
  CGUIControlGroupMask group(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
  auto* nestedGroup = new CGUIControlGroupMask(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
  auto* child = new CTestItemControl;
  nestedGroup->AddControl(child);
  group.AddControl(nestedGroup);
  CGUIListItem item;
  CDirtyRegionList dirtyRegions;

  group.UpdateVisibility(&item);
  group.Process(0, dirtyRegions);

  EXPECT_EQ(&item, child->visibilityItem);
}
