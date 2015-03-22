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
 *
 */
#pragma once

#include "threads/CriticalSection.h"

#include <map>
#include <vector>

class IJoystickInputHandler;

namespace PERIPHERALS { class CPeripheral; }

struct SPort
{
  IJoystickInputHandler* handler;
  unsigned int           deviceCount;
};

class CPortManager
{
private:
  CPortManager(void) { }

public:
  static CPortManager& Get(void);

  void OpenPort(IJoystickInputHandler* handler);
  void ClosePort(IJoystickInputHandler* handler);

  void GetPortMap(const std::vector<PERIPHERALS::CPeripheral*>& devices,
                  std::map<PERIPHERALS::CPeripheral*, IJoystickInputHandler*>& portMap) const;

private:
  // Utility functions
  static unsigned int GetNextOpenPort(const std::vector<SPort>& ports, unsigned int startPort = 0);
  static unsigned int GetMinDeviceCount(const std::vector<SPort>& ports);

  std::vector<SPort> m_ports;
  CCriticalSection   m_mutex;
};
