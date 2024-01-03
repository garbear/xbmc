/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentTopologyXML.h"

#include "AgentTopology.h"
#include "utils/log.h"

#include <tinyxml2.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto XML_ELM_TOPOLOGY = "logicaltopology";
constexpr auto XML_ELM_DEFINITION = "definition";
constexpr auto XML_ELM_GAMECLIENTS = "gameclients";
constexpr auto XML_ELM_GAMECLIENT = "gameclient";
constexpr auto XML_ATTR_TOPOLOGY_ID = "id";
constexpr auto XML_ATTR_SHA2_256 = "sha2-256";
constexpr auto XML_ATTR_DIGTEST_CREATION = "digestcreation";
} // namespace

bool CAgentTopologyXML::SerializeTopology(tinyxml2::XMLElement& topologyElement,
                                          const CAgentTopology& topology)
{
  // Set attribute "id" to ID
  topologyElement.SetAttribute(XML_ATTR_TOPOLOGY_ID, topology.GetID());

  // Set attribute "sha2-256" to digest
  topologyElement.SetAttribute(XML_ATTR_SHA2_256, topology.GetDigest().c_str());

  // Set attribute "digestcreation" to digest creation date
  topologyElement.SetAttribute(XML_ATTR_DIGTEST_CREATION,
                               topology.GetDigestCreationUTC().GetAsW3CDateTime(true).c_str());

  // Create "definition" element
  tinyxml2::XMLElement* definitionElement =
      topologyElement.InsertNewChildElement(XML_ELM_DEFINITION);

  // Serialize controller tree
  if (!topology.GetControllerTree().Serialize(*definitionElement))
    return false;

  // Create "gameclients" element
  tinyxml2::XMLElement* gameClientsElement =
      topologyElement.InsertNewChildElement(XML_ELM_GAMECLIENTS);

  // Serialize game clients
  const std::set<std::string>& gameClients = topology.GetGameClients();
  for (const std::string& gameClient : gameClients)
  {
    // Create "gameclient" element
    tinyxml2::XMLElement* gameClientElement =
        gameClientsElement->InsertNewChildElement(XML_ELM_GAMECLIENT);

    // Set value to ID of the game client
    gameClientElement->SetText(gameClient.c_str());
  }

  return true;
}

bool CAgentTopologyXML::DeserializeTopology(const tinyxml2::XMLElement& topologyElement,
                                            CAgentTopology& topology)
{
  // Deserialize ID
  const char* id = topologyElement.Attribute(XML_ATTR_TOPOLOGY_ID);
  if (id != nullptr)
  {
    int signedId = std::stoi(id);
    if (signedId < 0)
    {
      CLog::Log(LOGERROR, "Invalid attribute \"{}\": \"{}\"", XML_ATTR_TOPOLOGY_ID, id);
      return false;
    }

    topology.SetID(static_cast<unsigned int>(signedId));
  }

  // Deserialize digest
  const char* digest = topologyElement.Attribute(XML_ATTR_SHA2_256);
  if (digest != nullptr)
    topology.SetDigest(digest);

  // Deserialize digest creation date
  const char* digestCreation = topologyElement.Attribute(XML_ATTR_DIGTEST_CREATION);
  if (digestCreation != nullptr)
  {
    CDateTime digestCreationUtc;
    if (!digestCreationUtc.SetFromW3CDateTime(digestCreation, false))
    {
      CLog::Log(LOGERROR, "Invalid attribute \"{}\": \"{}\"", XML_ATTR_DIGTEST_CREATION,
                digestCreation);
      return false;
    }
    topology.SetDigestCreationUTC(digestCreationUtc);
  }

  // Get "definition" element
  const tinyxml2::XMLElement* definitionElement =
      topologyElement.FirstChildElement(XML_ELM_DEFINITION);
  if (definitionElement == nullptr)
  {
    CLog::Log(LOGERROR, "Missing element <{}>", XML_ELM_DEFINITION);
    return false;
  }

  // Deserialize controller tree
  if (!topology.DeserializeControllerTree(*definitionElement))
    return false;

  // Deserialize game clients
  std::set<std::string> gameClients;
  if (!DeserializeGameClients(topologyElement, gameClients))
    return false;

  // Set game clients
  topology.SetGameClients(std::move(gameClients));

  return true;
}

bool CAgentTopologyXML::DeserializeControllerTree(const CXBMCTinyXML2& xmlDoc,
                                                  CControllerTree& controllerTree)
{
  // Validate root element
  const tinyxml2::XMLElement* topologyElement = xmlDoc.RootElement();
  if (topologyElement == nullptr || std::strcmp(topologyElement->Value(), XML_ELM_TOPOLOGY) != 0)
  {
    CLog::Log(LOGERROR, "Can't find root <{}> tag", XML_ELM_TOPOLOGY);
    return false;
  }

  return DeserializeControllerTree(*topologyElement, controllerTree);
}

bool CAgentTopologyXML::DeserializeControllerTree(const tinyxml2::XMLElement& topologyElement,
                                                  CControllerTree& controllerTree)
{
  return controllerTree.Deserialize(topologyElement);
}

bool CAgentTopologyXML::DeserializeGameClients(const tinyxml2::XMLElement& topologyElement,
                                               std::set<std::string>& gameClients)
{
  // Get "gameclients" element
  const tinyxml2::XMLElement* gameClientsElement =
      topologyElement.FirstChildElement(XML_ELM_GAMECLIENTS);
  if (gameClientsElement == nullptr)
  {
    CLog::Log(LOGERROR, "Missing element <{}>", XML_ELM_GAMECLIENTS);
    return false;
  }

  // Get first "gameclient" element
  const tinyxml2::XMLElement* gameClientElement =
      gameClientsElement->FirstChildElement(XML_ELM_GAMECLIENT);
  if (gameClientElement == nullptr)
  {
    CLog::Log(LOGERROR, "Missing element <{}>", XML_ELM_GAMECLIENT);
    return false;
  }

  // Deserialize game clients
  while (gameClientElement != nullptr)
  {
    // Get ID of the game client
    const char* gameClientId = gameClientElement->GetText();
    if (gameClientId == nullptr)
      continue;

    // Add game client
    gameClients.insert(gameClientId);

    // Get next "gameclient" element
    gameClientElement = gameClientElement->NextSiblingElement(XML_ELM_GAMECLIENT);
  }

  return true;
}
