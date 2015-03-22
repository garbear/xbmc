/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 */

#include "PortManager.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "threads/SingleLock.h"

#include <algorithm>

using namespace PERIPHERALS;

// --- InputHandlerEqual -------------------------------------------------------

struct InputHandlerEqual
{
  InputHandlerEqual(IJoystickInputHandler* handler) : handler(handler) { }

  bool operator()(const SPort& port) { return port.handler == handler; }

  IJoystickInputHandler* const handler;
};

// --- CPortManager ------------------------------------------------------------

CPortManager& CPortManager::Get(void)
{
  static CPortManager instance;
  return instance;
}

void CPortManager::OpenPort(IJoystickInputHandler* handler)
{
  CSingleLock lock(m_mutex);

  SPort newPort = { handler, 0 };
  m_ports.push_back(newPort);
}

void CPortManager::ClosePort(IJoystickInputHandler* handler)
{
  CSingleLock lock(m_mutex);

  m_ports.erase(std::remove_if(m_ports.begin(), m_ports.end(), InputHandlerEqual(handler)), m_ports.end());
}

void CPortManager::GetPortMap(const std::vector<PERIPHERALS::CPeripheral*>& devices,
                              std::map<PERIPHERALS::CPeripheral*, IJoystickInputHandler*>& portMap) const
{
  std::vector<SPort> ports;

  {
    CSingleLock lock(m_mutex);
    ports = m_ports; // Modify copy so device count starts from zero next time
  }

  for (std::vector<CPeripheral*>::const_iterator it = devices.begin(); it != devices.end(); ++it)
  {
    int requestedPort = JOYSTICK_PORT_UNKNOWN;
    if ((*it)->Type() == PERIPHERAL_JOYSTICK)
    {
      CPeripheralJoystick* joystick = static_cast<CPeripheralJoystick*>(*it);
      if (joystick->RequestedPort() != JOYSTICK_PORT_UNKNOWN && joystick->RequestedPort() <= (int)m_ports.size())
        requestedPort = joystick->RequestedPort() - 1;
    }

    unsigned int targetPort = GetNextOpenPort(ports, requestedPort);

    portMap[*it] = ports[targetPort].handler;
    ports[targetPort].deviceCount++;
  }
}

unsigned int CPortManager::GetNextOpenPort(const std::vector<SPort>& ports, unsigned int startPort /* = 0 */)
{
  const unsigned int minDeviceCount = GetMinDeviceCount(ports);

  for (unsigned int i = 0, port = startPort; i < ports.size(); i++, port = (port + 1) % ports.size())
  {
    // Will match at least one port
    if (ports[port].deviceCount == minDeviceCount)
      return port;
  }

  return 0;
}

unsigned int CPortManager::GetMinDeviceCount(const std::vector<SPort>& ports)
{
  unsigned int minDeviceCount = 0;

  if (!ports.empty())
  {
    minDeviceCount = 9999; // large enough

    for (std::vector<SPort>::const_iterator itPort = ports.begin(); itPort != ports.end(); ++itPort)
      minDeviceCount = std::min(minDeviceCount, itPort->deviceCount);
  }

  return minDeviceCount;
}
