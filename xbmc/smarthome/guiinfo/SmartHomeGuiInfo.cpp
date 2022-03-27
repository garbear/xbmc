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
#include "smarthome/guiinfo/ILabHUD.h"
#include "smarthome/guiinfo/IStationHUD.h"
#include "smarthome/guiinfo/ISystemHealthHUD.h"
#include "utils/StringUtils.h"

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
                                     ILabHUD& labHud,
                                     IStationHUD& stationHud)
  : m_infoManager(infoManager),
    m_systemHealthHud(systemHealthHud),
    m_labHud(labHud),
    m_stationHud(stationHud)
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
    case SMARTHOME_LAB_CPU:
    {
      value = StringUtils::Format("{} %", m_labHud.CPUUtilization());
      return true;
    }
    case SMARTHOME_LAB_MEMORY:
    {
      value = StringUtils::Format("{} %", m_labHud.MemoryUtilization());
      return true;
    }
    case SMARTHOME_LAB_CURRENT:
    {
      value = StringUtils::Format("{} mA",
                                  static_cast<unsigned int>(m_labHud.ShuntCurrent() * 1000.0f));
      return true;
    }
    case SMARTHOME_LAB_IR:
    {
      value = StringUtils::Format("{:0.2f} V", m_labHud.IRVoltage());
      return true;
    }
    case SMARTHOME_STATION_SUPPLY:
    {
      value = StringUtils::Format("{:.1f} V", m_stationHud.SupplyVoltage());
      return true;
    }
    case SMARTHOME_STATION_MOTOR:
    {
      value = StringUtils::Format("{:.1f} V", m_stationHud.MotorVoltage());
      return true;
    }
    case SMARTHOME_STATION_CURRENT:
    {
      value = StringUtils::Format("{:.1f} A", m_stationHud.MotorCurrent());
      return true;
    }
    case SMARTHOME_STATION_CPU:
    {
      value = StringUtils::Format("{} %", m_stationHud.CPUPercent());
      return true;
    }
    case SMARTHOME_STATION_MESSAGE:
    {
      value = m_stationHud.Message();
      return true;
    }
    default:
      break;
  }

  return false;
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
    case SMARTHOME_HAS_LAB:
    {
      value = m_labHud.IsActive();
      return true;
    }
    case SMARTHOME_HAS_STATION:
    {
      value = m_stationHud.IsActive();
      return true;
    }
    default:
      break;
  }

  return false;
}
