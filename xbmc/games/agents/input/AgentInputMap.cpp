/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentInputMap.h"

#include "AgentInputMapXML.h"
#include "AgentTopology.h"
#include "URL.h"
#include "games/addons/GameClient.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <cstring>

using namespace KODI;
using namespace GAME;

namespace
{
// Application parameters
constexpr auto PROFILE_ROOT = "special://masterprofile";

// Game API parameters
constexpr auto TOPOLOGY_XML_FILE = "topology.xml";

// Input parameters
constexpr auto TOPOLOGIES_XML_FILE = "gametopologies.xml"; //! @todo Move to "games" subfolder
} // namespace

CAgentInputMap::CAgentInputMap()
  : m_topologiesXmlPath{URIUtils::AddFileToFolder(PROFILE_ROOT, TOPOLOGIES_XML_FILE)}
{
}

CAgentInputMap::~CAgentInputMap()
{
  // Wait for save tasks
  for (std::future<void>& task : m_saveFutures)
    task.wait();
  m_saveFutures.clear();
}

void CAgentInputMap::Clear()
{
  // Clear input parameters
  m_topologiesById.clear();
  m_topologiesByDigest.clear();
};

const CControllerTree& CAgentInputMap::GetAgentTopology(unsigned int topologyId) const
{
  const auto it = m_topologiesById.find(topologyId);
  if (it != m_topologiesById.end())
    return it->second->GetControllerTree();

  static const CControllerTree empty;
  return empty;
}

bool CAgentInputMap::AddGameClient(const GameClientPtr& gameClient)
{
  // Get path to game client's topologies.xml file
  std::string topologyXmlSharePath = URIUtils::AddFileToFolder(
      gameClient->Path(), GAME_CLIENT_RESOURCES_DIRECTORY, TOPOLOGY_XML_FILE);
  std::string topologyXmlLibPath = URIUtils::AddFileToFolder(
      gameClient->LibPath(), GAME_CLIENT_RESOURCES_DIRECTORY, TOPOLOGY_XML_FILE);

  std::string topologyXmlPath = topologyXmlSharePath;
  if (!CFileUtils::Exists(topologyXmlPath))
    topologyXmlPath = topologyXmlLibPath;

  if (!CFileUtils::Exists(topologyXmlPath))
  {
    CLog::Log(LOGDEBUG, "Can't load topologies, file doesn't exist");
    CLog::Log(LOGDEBUG, "  Tried: {}", CURL::GetRedacted(topologyXmlSharePath));
    CLog::Log(LOGDEBUG, "  Tried: {}", CURL::GetRedacted(topologyXmlLibPath));
    return false;
  }

  CLog::Log(LOGINFO, "{}: Loading topology: {}", gameClient->ID(),
            CURL::GetRedacted(topologyXmlPath));

  // Load topology
  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(topologyXmlPath))
  {
    CLog::Log(LOGDEBUG, "Unable to load file: {} at line {}", xmlDoc.ErrorStr(),
              xmlDoc.ErrorLineNum());
    return false;
  }

  // Deserialize topology
  std::shared_ptr<CAgentTopology> agentTopology = std::make_shared<CAgentTopology>();
  if (!agentTopology->DeserializeControllerTree(xmlDoc))
    return false;

  // Get topology digest
  agentTopology->UpdateDigest();
  const std::string& digest = agentTopology->GetDigest();

  // Check if topology digest already exists
  auto it = m_topologiesByDigest.find(digest);
  if (it != m_topologiesByDigest.end())
  {
    // Dereference iterator
    CAgentTopology& existingTopology = *it->second;

    // Add game client to existing topology
    const std::set<std::string>& gameClients = existingTopology.GetGameClients();
    if (gameClients.find(gameClient->ID()) == gameClients.end())
    {
      // Add game client
      existingTopology.AddGameClient(gameClient->ID());

      // Save topologies
      SaveXMLAsync();
    }
  }
  else
  {
    // Calculate next topology ID
    unsigned int nextTopologyId = 0;
    if (!m_topologiesById.empty())
      nextTopologyId = m_topologiesById.rbegin()->first + 1;

    // Set topology ID
    agentTopology->SetID(nextTopologyId);

    // Set first game client
    agentTopology->AddGameClient(gameClient->ID());

    // Add topology
    m_topologiesById[agentTopology->GetID()] = agentTopology;
    m_topologiesByDigest[agentTopology->GetDigest()] = std::move(agentTopology);

    // Save topologies
    SaveXMLAsync();
  }

  return true;
}

bool CAgentInputMap::LoadXML()
{
  Clear();

  // Check if topologies.xml file exists
  if (!CFileUtils::Exists(m_topologiesXmlPath))
  {
    CLog::Log(LOGDEBUG, "Can't load topologies, file doesn't exist: \"{}\"",
              CURL::GetRedacted(m_topologiesXmlPath));
    return false;
  }

  CLog::Log(LOGINFO, "Loading topologies: {}", CURL::GetRedacted(m_topologiesXmlPath));

  // Load topologies
  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(m_topologiesXmlPath))
  {
    CLog::Log(LOGDEBUG, "Unable to load file: {} at line {}", xmlDoc.ErrorStr(),
              xmlDoc.ErrorLineNum());
    return false;
  }

  // Deserialize topologies
  if (!CAgentInputMapXML::DeserializeTopologies(xmlDoc, m_topologiesById, m_topologiesByDigest))
    return false;

  return true;
}

void CAgentInputMap::SaveXMLAsync()
{
  TopologyIDMap topologies = m_topologiesById;

  // Prune any finished save tasks
  m_saveFutures.erase(std::remove_if(m_saveFutures.begin(), m_saveFutures.end(),
                                     [](std::future<void>& task) {
                                       return task.wait_for(std::chrono::seconds(0)) ==
                                              std::future_status::ready;
                                     }),
                      m_saveFutures.end());

  // Save async
  std::future<void> task = std::async(std::launch::async,
                                      [this, topologies = std::move(topologies)]()
                                      {
                                        CLog::Log(LOGDEBUG, "Saving topologies to {}",
                                                  CURL::GetRedacted(m_topologiesXmlPath));

                                        CXBMCTinyXML2 doc;
                                        if (CAgentInputMapXML::SerializeTopologies(doc, topologies))
                                        {
                                          std::lock_guard<std::mutex> lock(m_saveMutex);
                                          doc.SaveFile(m_topologiesXmlPath);
                                        }
                                      });

  m_saveFutures.emplace_back(std::move(task));
}
