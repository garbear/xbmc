/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "games/controllers/types/ControllerTree.h"

#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace tinyxml2
{
class XMLElement;
} // namespace tinyxml2

namespace KODI
{
namespace GAME
{
class CAgentTopology;

/*!
 * \ingroup games
 */
class CAgentInputMap
{
public:
  /*!
  * \brief ID -> topology map
  */
  using TopologyIDMap = std::map<unsigned int, std::shared_ptr<CAgentTopology>>;

  /*!
  * \brief Digest -> topology map
  */
  using TopologyDigestMap = std::map<std::string, std::shared_ptr<CAgentTopology>>;

  CAgentInputMap();
  ~CAgentInputMap();

  /*!
   * \brief Clean the input map
   */
  void Clear();

  /*!
   * \brief Get topology by ID
   */
  const CControllerTree& GetAgentTopology(unsigned int topologyId) const;

  /*!
   * \brief Add a game client to the input map
   */
  bool AddGameClient(const GameClientPtr& gameClient);

  // XML functions
  bool LoadXML();
  void SaveXMLAsync();

private:
  // Filesystem parameters
  std::string m_topologiesXmlPath;
  std::vector<std::future<void>> m_saveFutures;
  std::mutex m_saveMutex;

  // Input parameters
  TopologyIDMap m_topologiesById;
  TopologyDigestMap m_topologiesByDigest;
};
} // namespace GAME
} // namespace KODI
