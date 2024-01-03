/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "games/controllers/types/ControllerTree.h"
#include "utils/XBMCTinyXML2.h"

#include <memory>
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
/*!
 * \ingroup games
 */
class CAgentTopology
{
public:
  CAgentTopology();
  ~CAgentTopology() = default;

  void Clear();
  void UpdateDigest();

  // Topology property accessors
  unsigned int GetID() const { return m_id; }
  const std::string& GetDigest() const { return m_digest; }
  const CDateTime& GetDigestCreationUTC() const { return m_digestCreationUtc; }

  // Topology property mutators
  void SetID(unsigned int id) { m_id = id; }
  void SetDigest(const std::string& digest) { m_digest = digest; }
  void SetDigestCreationUTC(const CDateTime& digestCreationUtc)
  {
    m_digestCreationUtc = digestCreationUtc;
  }

  // Topology definition accessors
  const CControllerTree& GetControllerTree() const { return *m_controllerTree; }

  // Game property accessors
  const std::set<std::string>& GetGameClients() const { return m_gameClients; }

  // Game property mutators
  void SetGameClients(std::set<std::string> gameClients);
  void AddGameClient(std::string gameClientId);

  // XML functions
  bool SerializeTopology(tinyxml2::XMLElement& topologyElement) const;
  bool DeserializeTopology(const tinyxml2::XMLElement& topologyElement);
  bool DeserializeControllerTree(const CXBMCTinyXML2& xmlDoc);
  bool DeserializeControllerTree(const tinyxml2::XMLElement& topologyElement);
  bool DeserializeGameClients(const tinyxml2::XMLElement& topologyElement);

private:
  // Topology properties
  unsigned int m_id{0};
  std::string m_digest;
  CDateTime m_digestCreationUtc;

  // Topology definition
  std::unique_ptr<CControllerTree> m_controllerTree;

  // Game properties
  std::set<std::string> m_gameClients;
};
} // namespace GAME
} // namespace KODI
