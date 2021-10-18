/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerHub.h"

#include "ControllerTree.h"
#include "games/controllers/Controller.h"

#include <utility>

using namespace KODI;
using namespace GAME;

CControllerHub::~CControllerHub() = default;

CControllerHub& CControllerHub::operator=(const CControllerHub& rhs)
{
  if (this != &rhs)
  {
    m_ports = rhs.m_ports;
  }

  return *this;
}

void CControllerHub::Clear()
{
  m_ports.clear();
}

void CControllerHub::SetPorts(PortVec ports)
{
  m_ports = std::move(ports);
}

bool CControllerHub::IsControllerAccepted(const std::string& controllerId) const
{
  bool bAccepted = false;

  for (const CPortNode& port : m_ports)
  {
    if (port.IsControllerAccepted(controllerId))
    {
      bAccepted = true;
      break;
    }
  }

  return bAccepted;
}

bool CControllerHub::IsControllerAccepted(const std::string& portAddress,
                                          const std::string& controllerId) const
{
  bool bAccepted = false;

  for (const CPortNode& port : m_ports)
  {
    if (port.IsControllerAccepted(portAddress, controllerId))
    {
      bAccepted = true;
      break;
    }
  }

  return bAccepted;
}

ControllerVector CControllerHub::GetControllers() const
{
  ControllerVector controllers;
  GetControllers(controllers);
  return controllers;
}

void CControllerHub::GetControllers(ControllerVector& controllers) const
{
  for (const CPortNode& port : m_ports)
  {
    for (const CControllerNode& node : port.CompatibleControllers())
      node.GetControllers(controllers);
  }
}

const CPortNode& CControllerHub::GetPort(const std::string& address) const
{
  // Check children
  for (const CPortNode& port : m_ports)
  {
    const CPortNode& targetPort = GetPortInternal(port, address);
    if (targetPort.Address() == address)
      return targetPort;
  }

  // Not found
  static const CPortNode empty;
  return empty;
}

const CPortNode& CControllerHub::GetPortInternal(const CPortNode& port, const std::string& address)
{
  // Base case
  if (port.Address() == address)
    return port;

  // Check children
  for (const CControllerNode& controller : port.CompatibleControllers())
  {
    const PortVec& ports = controller.Hub().Ports();
    for (const CPortNode& port : ports)
    {
      const CPortNode& targetPort = GetPortInternal(port, address);
      if (targetPort.Address() == address)
        return targetPort;
    }
  }

  // Not found
  static const CPortNode empty{};
  return empty;
}
