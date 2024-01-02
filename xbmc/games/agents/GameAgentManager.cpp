/*
 *  Copyright (C) 2017-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameAgentManager.h"

#include "GameAgent.h"
#include "games/addons/GameClient.h"

using namespace KODI;
using namespace GAME;

CGameAgentManager::CGameAgentManager()
{
}

CGameAgentManager::~CGameAgentManager()
{
}

void CGameAgentManager::Start(GameClientPtr gameClient)
{
  // Initialize state
  m_gameClient = std::move(gameClient);
}

void CGameAgentManager::Stop()
{
  // Reset state
  m_gameClient.reset();
}

void CGameAgentManager::Refresh()
{
}

GameAgentVec CGameAgentManager::GetAgents() const
{
  std::lock_guard<std::mutex> lock(m_agentMutex);
  return m_agents;
}
