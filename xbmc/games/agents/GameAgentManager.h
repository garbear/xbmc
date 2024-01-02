/*
 *  Copyright (C) 2017-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "games/GameTypes.h"

#include <mutex>

namespace KODI
{
namespace GAME
{
class CGameClient;
class CGameClientJoystick;

/*!
 * \ingroup games
 *
 * \brief Class to manage game-playing agents
 */
class CGameAgentManager
{
public:
  CGameAgentManager();

  virtual ~CGameAgentManager();

  // Lifecycle functions
  void Start(GameClientPtr gameClient);
  void Stop();
  void Refresh();

  // Public interface
  GameAgentVec GetAgents() const;

private:
  // State parameters
  GameClientPtr m_gameClient;
  GameAgentVec m_agents;

  // Synchronization parameters
  mutable std::mutex m_agentMutex;
};
} // namespace GAME
} // namespace KODI
