/*
 *      Copyright (C) 2015-2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PortManager.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "peripherals/devices/PeripheralJoystickEmulation.h"
#include "threads/SingleLock.h"

#include <algorithm>
#include <array>

using namespace GAME;
using namespace JOYSTICK;
using namespace PERIPHERALS;

// --- GetRequestedPort() -----------------------------------------------------

namespace GAME
{
  int GetRequestedPort(const PERIPHERALS::CPeripheral* device)
  {
    switch (device->Type())
    {
    case PERIPHERAL_JOYSTICK:
      return static_cast<const CPeripheralJoystick*>(device)->RequestedPort();
    case PERIPHERAL_JOYSTICK_EMULATION:
      return static_cast<const CPeripheralJoystickEmulation*>(device)->ControllerNumber() - 1;
    default:
      break;
    }

    return JOYSTICK_PORT_UNKNOWN;
  }
}

// --- CPortManager -----------------------------------------------------------

CPortManager& CPortManager::GetInstance()
{
  static CPortManager instance;
  return instance;
}

void CPortManager::OpenPort(IInputHandler* handler, unsigned int port,
                            PERIPHERALS::PeripheralType device /* = PERIPHERALS::PERIPHERAL_UNKNOWN) */)
{
  CSingleLock lock(m_mutex);

  SPort newPort;
  newPort.handler = handler;
  newPort.port = port;
  newPort.type = device;
  m_ports.push_back(newPort);

  SetChanged();
  NotifyObservers(ObservableMessagePortsChanged);
}

void CPortManager::ClosePort(IInputHandler* handler)
{
  CSingleLock lock(m_mutex);

  m_ports.erase(std::remove_if(m_ports.begin(), m_ports.end(),
    [handler](const SPort& port)
    {
      return port.handler == handler;
    }), m_ports.end());

  SetChanged();
  NotifyObservers(ObservableMessagePortsChanged);
}

void CPortManager::MapDevices(const std::vector<CPeripheral*>& devices,
                              std::map<CPeripheral*, IInputHandler*>& deviceToPortMap)
{
  CSingleLock lock(m_mutex);

  if (m_ports.empty())
    return; // Nothing to do

  // Clear all ports
  for (SPort& port : m_ports)
    port.device = nullptr;

  // Separate devices by type
  std::map<PeripheralType, std::vector<CPeripheral*>> devicesByType = SplitDevices(devices);

  for (auto it = devicesByType.begin(); it != devicesByType.end(); ++it)
  {
    PeripheralType type = it->first;
    std::vector<CPeripheral*>& devicesOfType = it->second;

    // Get devices requesting a prt
    std::vector<CPeripheral*> requestedPort;
    for (CPeripheral* device : devices)
    {
      if (GetRequestedPort(device) != JOYSTICK_PORT_UNKNOWN)
        requestedPort.push_back(device);
    }

    // Assign requested ports
    for (CPeripheral* device : requestedPort)
    {
      IInputHandler* handler = AssignToPort(device);
      if (handler)
      {
        deviceToPortMap[device] = handler;
        devicesOfType.erase(std::remove(devicesOfType.begin(), devicesOfType.end(), device), devicesOfType.end());
      }
    }

    // Assign remaining ports
    for (CPeripheral* device : devicesOfType)
    {
      IInputHandler* handler = AssignToPort(device);
      if (handler)
        deviceToPortMap[device] = handler;
    }
  }
}

IInputHandler* CPortManager::AssignToPort(CPeripheral* device)
{
  const int requestedPort = GetRequestedPort(device);

  for (SPort& port : m_ports)
  {
    // Skip occupied ports
    if (port.device != nullptr)
      continue;

    // Skip non-requested ports
    if (requestedPort != JOYSTICK_PORT_UNKNOWN && requestedPort != port.port)
      continue;

    // Filter by type
    if (port.type != PERIPHERAL_UNKNOWN && port.type != device->Type())
      continue;

    // Success
    port.device = device;
    return port.handler;
  }
  return nullptr;
}

CPortManager::DeviceMap CPortManager::SplitDevices(const std::vector<CPeripheral*>& devices)
{
  DeviceMap devicesByType;

  std::array<PeripheralType, 2> Types = { PERIPHERAL_JOYSTICK, PERIPHERAL_JOYSTICK_EMULATION };

  for (PeripheralType type : Types)
  {
    std::vector<CPeripheral*> devicesOfType = devices;
    devicesOfType.erase(std::remove_if(devicesOfType.begin(), devicesOfType.end(),
      [type](const CPeripheral* peripheral)
    {
      return peripheral->Type() != type;
    }), devicesOfType.end());

    devicesByType.insert(DeviceMap::value_type(type, devicesOfType));
  }

  return devicesByType;
}
