/*
 *  Copyright (C) 2022-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeGuiInfo.h"

#include "GUIInfoManager.h"
#include "LangInfo.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "smarthome/guiinfo/IPowerMeterHUD.h"
#include "smarthome/guiinfo/ISystemHealthHUD.h"
#include "smarthome/guiinfo/SmartHomeProperty.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <chrono>
#include <cmath>
using namespace KODI;
using namespace SMART_HOME;

namespace
{
std::string FormatFrequency(double frequencyHz)
{
  if (frequencyHz < 0.0)
    return "";

  constexpr double kHz = 1'000.0;
  constexpr double MHz = 1'000'000.0;
  constexpr double GHz = 1'000'000'000.0;

  if (frequencyHz >= GHz)
    return StringUtils::Format("{:.2f} GHz", frequencyHz / GHz);

  if (frequencyHz >= MHz)
    return StringUtils::Format("{:.1f} MHz", frequencyHz / MHz);

  if (frequencyHz >= kHz)
    return StringUtils::Format("{:.0f} KHz", frequencyHz / kHz);

  return StringUtils::Format("{:.0f} Hz", frequencyHz);
}
} // namespace

CSmartHomeGuiInfo::CSmartHomeGuiInfo(CGUIInfoManager& infoManager,
                                     ISystemHealthHUD& systemHealthHud,
                                     IPowerMeterHUD& powerMeterHud)
  : m_infoManager(infoManager),
    m_systemHealthHud(systemHealthHud),
    m_powerMeterHud(powerMeterHud)
{
}

CSmartHomeGuiInfo::~CSmartHomeGuiInfo() = default;

void CSmartHomeGuiInfo::Initialize()
{
  m_infoManager.RegisterInfoProvider(this);
}

void CSmartHomeGuiInfo::Deinitialize()
{
  m_infoManager.UnregisterInfoProvider(this);
}

bool CSmartHomeGuiInfo::GetLabel(std::string& value,
                                 const CFileItem* item,
                                 int contextWindow,
                                 const GUILIB::GUIINFO::CGUIInfo& info,
                                 std::string* fallback) const
{
  switch (info.GetInfo())
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SMARTHOME_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SMARTHOME_PROPERTY:
    {
      const auto type = SmartHomePropertyTypeFromString(info.GetData6());
      if (!type)
        break;

      const auto property = m_systemHealthHud.Property(info.GetData3(), info.GetData5(), *type);
      if (!property)
      {
        value.clear();
        return true;
      }

      FormatSmartHomeProperty(*property, info.GetData7(), value);
      return true;
    }
    case SMARTHOME_POWER_METER_VOLTAGE:
    case SMARTHOME_POWER_METER_CURRENT:
    case SMARTHOME_POWER_METER_POWER:
    {
      const auto reading = m_powerMeterHud.PowerMeter(info.GetData3(), info.GetData5());
      if (!reading)
      {
        value.clear();
        return true;
      }

      double number;
      switch (info.GetInfo())
      {
        case SMARTHOME_POWER_METER_VOLTAGE:
          number = reading->voltage;
          break;
        case SMARTHOME_POWER_METER_CURRENT:
          number = reading->current;
          break;
        default:
          number = reading->power;
          break;
      }

      FormatSmartHomeNumber(number, info.GetData7(), value);
      return true;
    }
    case SMARTHOME_POWER_METER_CURRENT_SHARE:
    {
      const auto share =
          m_powerMeterHud.CurrentShare(info.GetData3(), info.GetData5(), info.GetData6());
      if (!share)
      {
        value.clear();
        return true;
      }

      FormatSmartHomeNumber(*share, info.GetData7(), value);
      return true;
    }
    case SMARTHOME_CPU_TEMPERATURE:
    {
      const std::string systemName = info.GetData3();
      if (!systemName.empty())
      {
        const CTemperature cpuTemperature = m_systemHealthHud.CPUTemperature(systemName);
        value = cpuTemperature.IsValid() ? g_langInfo.GetTemperatureAsString(cpuTemperature) : "";
        return true;
      }
      break;
    }
    case SMARTHOME_CPU_UTILIZATION:
    {
      const std::string systemName = info.GetData3();
      if (!systemName.empty())
      {
        const float cpuUtilization = m_systemHealthHud.CPUUtilization(systemName);
        value = StringUtils::Format("{} %", static_cast<unsigned int>(std::round(cpuUtilization)));
        return true;
      }
      break;
    }
    case SMARTHOME_CPU_FREQUENCY:
    {
      const std::string systemName = info.GetData3();
      if (!systemName.empty())
      {
        const double cpuFrequencyHz = m_systemHealthHud.CPUFrequencyHz(systemName);
        value = FormatFrequency(cpuFrequencyHz);
        return true;
      }
      break;
    }
    case SMARTHOME_RAM_UTILIZATION:
    {
      const std::string systemName = info.GetData3();
      if (!systemName.empty())
      {
        const float memoryUtilization = m_systemHealthHud.MemoryUtilization(systemName);
        value =
            StringUtils::Format("{} %", static_cast<unsigned int>(std::round(memoryUtilization)));
        return true;
      }
      break;
    }
    case SMARTHOME_BATTERY_CHARGE:
    {
      const std::string systemName = info.GetData3();
      if (!systemName.empty())
      {
        const unsigned int batteryCharge = m_systemHealthHud.BatteryCharge(systemName);
        value = StringUtils::Format("{} %", batteryCharge);
        return true;
      }
      break;
    }
    case SMARTHOME_BATTERY_LOAD:
    {
      const std::string systemName = info.GetData3();
      if (!systemName.empty())
      {
        const float batteryLoadWatts = m_systemHealthHud.BatteryLoad(systemName);
        value =
            StringUtils::Format("{} W", static_cast<unsigned int>(std::round(batteryLoadWatts)));
        return true;
      }
      break;
    }
    default:
      break;
  }

  return false;
}

bool CSmartHomeGuiInfo::GetInt(int& value,
                               const CGUIListItem* item,
                               int contextWindow,
                               const GUILIB::GUIINFO::CGUIInfo& info) const
{
  if (info.GetInfo() != SMARTHOME_POWER_METER_CURRENT_SHARE)
    return false;

  const auto share =
      m_powerMeterHud.CurrentShare(info.GetData3(), info.GetData5(), info.GetData6());
  if (!share)
    return false;

  value = std::clamp(static_cast<int>(std::lround(*share)), 0, 100);
  return true;
}

bool CSmartHomeGuiInfo::GetBool(bool& value,
                                const CGUIListItem* item,
                                int contextWindow,
                                const GUILIB::GUIINFO::CGUIInfo& info) const
{
  switch (info.GetInfo())
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SMARTHOME_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SMARTHOME_IS_ACTIVE:
    {
      const std::string systemName = info.GetData3();
      if (!systemName.empty())
      {
        const int timeoutMs = info.GetData4();
        if (timeoutMs > 0)
          value = m_systemHealthHud.IsActive(systemName, std::chrono::milliseconds(timeoutMs));
        else
          value = m_systemHealthHud.IsActive(systemName);
        return true;
      }
      break;
    }
    default:
      break;
  }

  return false;
}
