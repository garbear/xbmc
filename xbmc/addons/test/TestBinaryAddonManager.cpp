/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "addons/binary-addons/BinaryAddonManager.h"

#include <gtest/gtest.h>

#include <mutex>

namespace ADDON
{
class CBinaryAddonManagerTestAccess
{
public:
  static void Publish(CBinaryAddonManager& manager,
                      const std::shared_ptr<CBinaryAddonBase>& base)
  {
    std::unique_lock lock(manager.m_critSection);
    manager.m_runningAddons.insert_or_assign(base->ID(), base);
  }
};
} // namespace ADDON

TEST(TestBinaryAddonManager, StaleGenerationReleaseCannotEraseReplacement)
{
  using namespace ADDON;
  CBinaryAddonManager manager;
  const auto info = CAddonInfoBuilder::Generate("game.shader.test", AddonType::SHADERDLL);
  ASSERT_TRUE(info);
  auto oldGeneration = std::make_shared<CBinaryAddonBase>(info);
  auto newGeneration = std::make_shared<CBinaryAddonBase>(info);
  CBinaryAddonManagerTestAccess::Publish(manager, newGeneration);

  manager.ReleaseAddonBase(oldGeneration, nullptr);

  EXPECT_EQ(newGeneration, manager.GetRunningAddonBase("game.shader.test"));
}
