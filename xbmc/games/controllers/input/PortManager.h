/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/types/ControllerTree.h"

class TiXmlElement;

namespace KODI
{
namespace GAME
{
/*
class CControllerState;

class CPortState
{
public:
  CPortState(const std::string& defaultController);

  bool IsDefault() const
  {
    if (!connected)
      return false;

    for (const CControllerState& controller : m_controllerStates)
    {
      if (!controller.IsDefault())
        return false;
    }

    return true;
  }

  std::string portId;
  std::string portAddress;
  bool connected;
  std::string controllerId; // Empty if disconnected
  std::vector<CControllerState> m_controllerStates;
};

class CControllerState
{
public:
  bool IsDefault() const
  {
    for (const CPortState& port : m_portStates)
    {
      if (!port.IsDefault())
        return false;
    }

    return true;
  }

  std::vector<CPortState> m_portStates;
};
*/

class CPortManager
{
public:
  CPortManager();
  ~CPortManager();

  void Initialize(const std::string& profilePath);
  void Deinitialize() {}

  void SetControllerTree(const CControllerTree& controllerTree);
  void LoadXML();
  void SaveXML();
  void Clear();

  void ConnectController(const std::string& portAddress,
                         bool connected,
                         const std::string& controllerId = "");

  const CControllerTree& ControllerTree() const { return m_controllerTree; }

private:
  static void DeserializePorts(const TiXmlElement* pElement, PortVec& ports);
  static void DeserializePort(const TiXmlElement* pElement, CPortNode& port);
  static void DeserializeControllers(const TiXmlElement* pElement, ControllerNodeVec& controllers);
  static void DeserializeController(const TiXmlElement* pElement, CControllerNode& controller);

  static void SerializePorts(TiXmlElement& node, const PortVec& ports);
  static void SerializePort(TiXmlElement& portNode, const CPortNode& port);
  static void SerializeControllers(TiXmlElement& portNode, const ControllerNodeVec& controllers);
  static void SerializeController(TiXmlElement& controllerNode, const CControllerNode& controller);

  static bool ConnectController(const std::string& portAddress,
                                bool connected,
                                const std::string& controllerId,
                                PortVec& ports);
  static bool ConnectController(const std::string& portAddress,
                                bool connected,
                                const std::string& controllerId,
                                CPortNode& port);
  static bool ConnectController(const std::string& portAddress,
                                bool connected,
                                const std::string& controllerId,
                                ControllerNodeVec& controllers);
  static bool ConnectController(const std::string& portAddress,
                                bool connected,
                                const std::string& controllerId,
                                CControllerNode& controller);

  CControllerTree m_controllerTree;
  std::string m_xmlPath;
};
} // namespace GAME
} // namespace KODI
