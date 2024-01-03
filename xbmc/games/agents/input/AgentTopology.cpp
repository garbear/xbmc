/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentTopology.h"

#include "AgentTopologyXML.h"
#include "utils/Digest.h"

using namespace KODI;
using namespace GAME;

CAgentTopology::CAgentTopology() : m_controllerTree(std::make_unique<CControllerTree>())
{
}

void CAgentTopology::Clear()
{
  m_controllerTree->Clear();
  m_gameClients.clear();
}

void CAgentTopology::UpdateDigest()
{
  if (m_digest.empty() || !m_digestCreationUtc.IsValid())
  {
    // Calculate digest
    std::string strDigest = m_controllerTree->GetDigest(UTILITY::CDigest::Type::SHA256);
    m_digest = StringUtils::ToHexadecimal(strDigest);

    // Take a timestamp of the system clock
    m_digestCreationUtc = CDateTime::GetUTCDateTime();
  }
}

void CAgentTopology::SetGameClients(std::set<std::string> gameClients)
{
  m_gameClients = std::move(gameClients);
}

void CAgentTopology::AddGameClient(std::string gameClientId)
{
  m_gameClients.insert(std::move(gameClientId));
}

bool CAgentTopology::SerializeTopology(tinyxml2::XMLElement& topologyElement) const
{
  return CAgentTopologyXML::SerializeTopology(topologyElement, *this);
}

bool CAgentTopology::DeserializeTopology(const tinyxml2::XMLElement& topologyElement)
{
  return CAgentTopologyXML::DeserializeTopology(topologyElement, *this);
}

bool CAgentTopology::DeserializeControllerTree(const CXBMCTinyXML2& xmlDoc)
{
  return CAgentTopologyXML::DeserializeControllerTree(xmlDoc, *m_controllerTree);
}

bool CAgentTopology::DeserializeControllerTree(const tinyxml2::XMLElement& topologyElement)
{
  return CAgentTopologyXML::DeserializeControllerTree(topologyElement, *m_controllerTree);
}

bool CAgentTopology::DeserializeGameClients(const tinyxml2::XMLElement& topologyElement)
{
  return CAgentTopologyXML::DeserializeGameClients(topologyElement, m_gameClients);
}
