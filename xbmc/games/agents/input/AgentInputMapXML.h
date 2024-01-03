/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AgentInputMap.h"
#include "utils/XBMCTinyXML2.h"

namespace KODI
{
namespace GAME
{
class CAgentTopology;

/*!
 * \ingroup games
 */
class CAgentInputMapXML
{
public:
  static bool SerializeTopologies(CXBMCTinyXML2& xmlDoc,
                                  const CAgentInputMap::TopologyIDMap& topologies);
  static bool SerializeTopology(tinyxml2::XMLElement& topologyElement,
                                const CAgentTopology& topology);

  static bool DeserializeTopologies(const CXBMCTinyXML2& xmlDoc,
                                    CAgentInputMap::TopologyIDMap& topologiesById,
                                    CAgentInputMap::TopologyDigestMap& topologiesByDigest);
  static bool DeserializeTopology(const tinyxml2::XMLElement& topologyElement,
                                  CAgentTopology& topology);
};
} // namespace GAME
} // namespace KODI
