/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentInputMapXML.h"

#include "AgentTopology.h"
#include "utils/log.h"

#include <cstring>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto XML_ELM_TOPOLOGIES = "topologies";
constexpr auto XML_ELM_TOPOLOGY = "logicaltopology";
} // namespace

bool CAgentInputMapXML::SerializeTopologies(CXBMCTinyXML2& doc,
                                            const CAgentInputMap::TopologyIDMap& topologies)
{
  // Create root topologies element
  tinyxml2::XMLElement* topologiesElement = doc.NewElement(XML_ELM_TOPOLOGIES);
  if (topologiesElement == nullptr)
    return false;

  // Iterate over all topologies
  for (const auto& topology : topologies)
  {
    // Dereference iterator
    const CAgentTopology& agentTopology = *topology.second;

    // Create topology element
    tinyxml2::XMLElement* topologyElement =
        topologiesElement->InsertNewChildElement(XML_ELM_TOPOLOGY);

    // Serialize topology
    if (!SerializeTopology(*topologyElement, agentTopology))
      continue;
  }

  // Add topologies element
  doc.InsertEndChild(topologiesElement);

  return true;
}

bool CAgentInputMapXML::SerializeTopology(tinyxml2::XMLElement& topologyElement,
                                          const CAgentTopology& topology)
{
  return topology.SerializeTopology(topologyElement);
}

bool CAgentInputMapXML::DeserializeTopologies(const CXBMCTinyXML2& xmlDoc,
                                              CAgentInputMap::TopologyIDMap& topologiesById,
                                              CAgentInputMap::TopologyDigestMap& topologiesByDigest)
{
  // Validate root element
  const tinyxml2::XMLElement* topologiesElement = xmlDoc.RootElement();
  if (topologiesElement == nullptr ||
      std::strcmp(topologiesElement->Value(), XML_ELM_TOPOLOGIES) != 0)
  {
    CLog::Log(LOGERROR, "Can't find root <{}> tag", XML_ELM_TOPOLOGIES);
    return false;
  }

  // Get first "logicaltopology" element
  const tinyxml2::XMLElement* topologyElement =
      topologiesElement->FirstChildElement(XML_ELM_TOPOLOGY);
  if (topologyElement == nullptr)
  {
    CLog::Log(LOGERROR, "Missing element <{}>", XML_ELM_TOPOLOGY);
    return false;
  }

  // Iterate over all "logicaltopology" elements
  while (topologyElement != nullptr)
  {
    // Create topology
    std::shared_ptr<CAgentTopology> topology = std::make_shared<CAgentTopology>();
    if (DeserializeTopology(*topologyElement, *topology))
    {
      // Check topology ID
      if (topologiesById.find(topology->GetID()) != topologiesById.end())
      {
        CLog::Log(LOGERROR, "Duplicate topology ID with ID {} and digest {}", topology->GetID(),
                  topology->GetDigest());
      }
      else
      {
        // Add topology
        topologiesById[topology->GetID()] = topology;
        topologiesByDigest[topology->GetDigest()] = std::move(topology);
      }
    }

    // Get next "logicaltopology" element
    topologyElement = topologyElement->NextSiblingElement(XML_ELM_TOPOLOGY);
  }

  return true;
}

bool CAgentInputMapXML::DeserializeTopology(const tinyxml2::XMLElement& topologyElement,
                                            CAgentTopology& topology)
{
  return topology.DeserializeTopology(topologyElement);
}
