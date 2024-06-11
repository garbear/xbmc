/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIListItem.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabel.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "guilib/guiinfo/GUIInfoProvider.h"
#include "guilib/guiinfo/GUIInfoProviders.h"
#include "smarthome/guiinfo/SmartHomeProperty.h"
#include "utils/Variant.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI::SMART_HOME;

namespace
{
class TestGUIComponent : public CGUIComponent
{
public:
  TestGUIComponent() : CGUIComponent(false)
  {
    m_guiInfoManager = std::make_unique<CGUIInfoManager>();
    CServiceBroker::RegisterGUI(this);
  }
  ~TestGUIComponent() override { CServiceBroker::UnregisterGUI(); }
};

class TestPropertyProvider : public KODI::GUILIB::GUIINFO::CGUIInfoProvider
{
public:
  bool InitCurrentItem(CFileItem*) override { return false; }

  bool GetLabel(std::string& value,
                const CFileItem*,
                int,
                const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                std::string*) const override
  {
    if (info.GetInfo() == SMARTHOME_PROPERTY)
    {
      EXPECT_EQ("station", info.GetData3());
      EXPECT_EQ("traction_voltage", info.GetData5());
      EXPECT_EQ("float32", info.GetData6());
      EXPECT_TRUE(info.GetData7().empty() || info.GetData7() == "{:.1f} V");
      return FormatSmartHomeProperty(7.843f, info.GetData7(), value);
    }
    if (info.GetInfo() == SYSTEM_BUILD_VERSION)
    {
      value = "22.0";
      return true;
    }
    return false;
  }

  bool GetInt(int&, const CGUIListItem*, int, const KODI::GUILIB::GUIINFO::CGUIInfo&) const override
  {
    return false;
  }

  bool GetBool(bool&,
               const CGUIListItem*,
               int,
               const KODI::GUILIB::GUIINFO::CGUIInfo&) const override
  {
    return false;
  }
};

} // namespace

TEST(TestSmartHomeProperty, Translation)
{
  CGUIInfoManager infoManager;
  EXPECT_NE(0, infoManager.TranslateString(
                   "SmartHome.System(station).Property(traction_voltage,float32)"));
  EXPECT_NE(0, infoManager.TranslateString(
                   "SmartHome.System(station).Property(traction_voltage,float32,{:.1f} V)"));
  EXPECT_NE(0, infoManager.TranslateString("SmartHome.System(station).IsActive(2)"));
}

TEST(TestSmartHomeProperty, ListItemPropertyFloat)
{
  CGUIInfoManager infoManager;
  CGUIListItem item;
  item.SetProperty("smarthome.current_balance", CVariant{47.625f});

  const int info = infoManager.TranslateString("ListItem.Property(smarthome.current_balance)");
  ASSERT_NE(0, info);

  float value = 0.0f;
  EXPECT_TRUE(infoManager.GetFloat(value, info, 0, &item));
  EXPECT_FLOAT_EQ(47.625f, value);
}

TEST(TestSmartHomeProperty, InvalidTranslation)
{
  CGUIInfoManager infoManager;
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System().Property(value,float32)"));
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System(station).Property(,float32)"));
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System(station).Property(value)"));
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System(station).Property(value,Float32)"));
  EXPECT_EQ(0,
            infoManager.TranslateString("SmartHome.System(station).Property(value,float32,{},x)"));
  EXPECT_EQ(0,
            infoManager.TranslateString("SmartHome.System(station).Property(value,float32,{:.1f)"));
}

TEST(TestSmartHomeProperty, InfoLabelNestedCommasAndOuterAffixes)
{
  TestGUIComponent gui;
  TestPropertyProvider provider;
  gui.GetInfoManager().GetInfoProviders().RegisterProvider(&provider, false);

  KODI::GUILIB::GUIINFO::CGUIInfoLabel label("$INFO[SmartHome.System(station).Property(traction_"
                                             "voltage,float32,{:.1f} V),prefix:, postfix]");
  EXPECT_EQ("prefix:7.8 V postfix", label.GetLabel(0));

  KODI::GUILIB::GUIINFO::CGUIInfoLabel defaultFormat(
      "$INFO[SmartHome.System(station).Property(traction_voltage,float32)]");
  EXPECT_EQ("7.843", defaultFormat.GetLabel(0));

  KODI::GUILIB::GUIINFO::CGUIInfoLabel existing("$INFO[System.BuildVersion,prefix:, postfix]");
  EXPECT_EQ("prefix:22.0 postfix", existing.GetLabel(0));

  KODI::GUILIB::GUIINFO::CGUIInfoLabel malformed(
      "$INFO[SmartHome.System(station).Property(traction_voltage,float32,prefix:,postfix]");
  EXPECT_TRUE(malformed.IsConstant());

  gui.GetInfoManager().GetInfoProviders().UnregisterProvider(&provider);
}

TEST(TestSmartHomeProperty, ScalarTypes)
{
  constexpr std::array<std::string_view, 12> types{"bool",   "int8",    "int16",   "int32",
                                                   "int64",  "uint8",   "uint16",  "uint32",
                                                   "uint64", "float32", "float64", "string"};
  for (const auto type : types)
    EXPECT_TRUE(SmartHomePropertyTypeFromString(type).has_value()) << type;

  EXPECT_FALSE(SmartHomePropertyTypeFromString("Byte").has_value());
  EXPECT_FALSE(SmartHomePropertyTypeFromString("array").has_value());
}

TEST(TestSmartHomeProperty, Formatting)
{
  std::string result;
  EXPECT_TRUE(FormatSmartHomeProperty(7.843f, "{:.1f} V", result));
  EXPECT_EQ("7.8 V", result);
  EXPECT_TRUE(FormatSmartHomeProperty(uint32_t{42}, {}, result));
  EXPECT_EQ("42", result);
  EXPECT_FALSE(FormatSmartHomeProperty(7.843f, "{:.1f", result));
  EXPECT_TRUE(result.empty());
}
