/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/types/ControllerTree.h"
#include "utils/XBMCTinyXML2.h"

#include <set>
#include <string>

namespace tinyxml2
{
class XMLElement;
}

namespace KODI
{
namespace GAME
{
class CAgentTopology;

/*!
 * \ingroup games
 */
class CAgentTopologyXML
{
public:
  static bool SerializeTopology(tinyxml2::XMLElement& topologyElement,
                                const CAgentTopology& topology);

  static bool DeserializeTopology(const tinyxml2::XMLElement& topologyElement,
                                  CAgentTopology& topology);
  static bool DeserializeControllerTree(const CXBMCTinyXML2& xmlDoc,
                                        CControllerTree& controllerTree);
  static bool DeserializeControllerTree(const tinyxml2::XMLElement& topologyElement,
                                        CControllerTree& controllerTree);
  static bool DeserializeGameClients(const tinyxml2::XMLElement& topologyElement,
                                     std::set<std::string>& gameClients);
};
} // namespace GAME
} // namespace KODI
